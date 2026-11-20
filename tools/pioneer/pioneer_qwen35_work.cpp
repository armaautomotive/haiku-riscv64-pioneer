/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>

#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35WorkProtocol.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQwen35Preload.h"
#include "llama.h"

namespace D=PioneerQwen35;

// Load only the vocabulary API. Work creates no llama context, backend
// instance, weight buffer, scheduler or CPU inference path.
class Vocabulary {
	void* library=nullptr;
	llama_model* model=nullptr;
	const llama_vocab* vocab=nullptr;
	decltype(&llama_model_free) freeModel=nullptr;
	decltype(&llama_tokenize) tokenize=nullptr;
	decltype(&llama_detokenize) detokenize=nullptr;

	template<class Function> Function Resolve(const char* name) const
	{
		return reinterpret_cast<Function>(dlsym(library,name));
	}
public:
	~Vocabulary()
	{
		if (model) freeModel(model);
		if (library) dlclose(library);
	}

	bool Open(const char* file,FILE* source)
	{
		if (!file || file[0]!='/' || fseeko(source,0,SEEK_SET)) return false;
		library=dlopen(file,RTLD_NOW|RTLD_LOCAL);
		if (!library) {
			fprintf(stderr,"Vocabulary library: %s\n",dlerror());
			return false;
		}
		auto defaults=Resolve<decltype(&llama_model_default_params)>("llama_model_default_params");
		auto load=Resolve<decltype(&llama_model_load_from_file_ptr)>("llama_model_load_from_file_ptr");
		auto get=Resolve<decltype(&llama_model_get_vocab)>("llama_model_get_vocab");
		auto size=Resolve<decltype(&llama_vocab_n_tokens)>("llama_vocab_n_tokens");
		auto eos=Resolve<decltype(&llama_vocab_eos)>("llama_vocab_eos");
		auto text=Resolve<decltype(&llama_vocab_get_text)>("llama_vocab_get_text");
		freeModel=Resolve<decltype(freeModel)>("llama_model_free");
		tokenize=Resolve<decltype(tokenize)>("llama_tokenize");
		detokenize=Resolve<decltype(detokenize)>("llama_detokenize");
		if (!defaults || !load || !get || !size || !eos || !text || !freeModel || !tokenize || !detokenize)
			return false;
		auto params=defaults();
		ggml_backend_dev_t none[]={nullptr};
		params.devices=none;
		params.n_gpu_layers=0;
		params.load_mode=LLAMA_LOAD_MODE_NONE;
		params.vocab_only=true;
		model=load(source,params);
		if (!model) return false;
		vocab=get(model);
		return vocab && size(vocab)==D::Vocabulary && eos(vocab)==D::EndOfTurn
			&& !strcmp(text(vocab,D::EndOfText),"<|endoftext|>")
			&& !strcmp(text(vocab,D::EndOfTurn),"<|im_end|>");
	}

	bool Tokenize(const char* prompt,D::WorkStart& request,unsigned context) const
	{
		const size_t length=strlen(prompt);
		if (!length || length>INT_MAX) return false;
		const bool chatml=length>=sizeof("<|im_start|>")-1
			&& !memcmp(prompt,"<|im_start|>",sizeof("<|im_start|>")-1);
		std::vector<llama_token> ids(context);
		// Structured Qwen prompts carry their own special-token boundaries and
		// must not receive an extra BOS. Keep plain-prefix diagnostics unchanged.
		const int count=tokenize(vocab,prompt,int(length),ids.data(),ids.size(),!chatml,chatml);
		if (count<=0 || unsigned(count)>context) return false;
		request.promptCount=unsigned(count);
		for (int i=0; i<count; i++) request.prompt[i]=unsigned(ids[i]);
		return true;
	}

	bool Detokenize(const D::WorkPoll& result,std::string& text) const
	{
		const auto count=result.progress.generated;
		std::vector<llama_token> ids(result.generatedIds,result.generatedIds+count);
		int needed=detokenize(vocab,ids.data(),ids.size(),nullptr,0,true,false);
		if (needed>=0 || needed==INT_MIN) return false;
		text.resize(unsigned(-needed));
		return detokenize(vocab,ids.data(),ids.size(),text.data(),text.size(),true,false)==-needed
			&& !text.empty();
	}
};

static bool Call(int fd,uint32 op,void* data,size_t bytes)
{
	if (!ioctl(fd,op,data,bytes)) return true;
	fprintf(stderr,"P100A_Q35 ioctl=%08x error=%s\n",op,strerror(errno));
	return false;
}

static unsigned Parse(const char* text,unsigned maximum)
{
	char* end=nullptr;
	if (!text || *text<'0' || *text>'9') return 0;
	errno=0;
	unsigned long value=strtoul(text,&end,10);
	return errno || *end || value>maximum?0:unsigned(value);
}

static bool Begin(int fd,bool& bankOpen)
{
	bankOpen=false;
	D::WorkSupport work={};
	tt_q38_span_support span={};
	tt_q38_bank_support bank={};
	if (!Call(fd,TT_PCI_PROBE_Q35_WORK_SUPPORT,&work,sizeof(work))
		|| work.version!=D::WorkVersion || work.driver!=D::WorkDriver || !work.tensixProjections
		|| work.hostInference || !work.explicitEnd || work.reserved || work.vocabulary!=D::Vocabulary
		|| work.maxContext!=D::MaxPositions
		|| !Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&bank,sizeof(bank))
		|| bank.driver!=TT_Q38_BANK_DRIVER || bank.version!=TT_Q38_BANK_PROTOCOL
		|| bank.banks!=D::Banks || bank.page_bytes!=D::LoadPageBytes || !bank.storage_only
		|| bank.bytes_per_bank!=PioneerQwen35PackedPlan::Capacity
		|| !Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,&span,sizeof(span))
		|| span.version!=TT_Q38_SPAN_PUBLIC_VERSION || span.banks!=D::Banks
		|| span.page_bytes!=D::LoadPageBytes || span.max_bytes!=D::LoadSpanBytes
		|| !span.before_cohort || !span.explicit_end || span.reserved) return false;
	tt_q38_bank_session session={}; session.version=TT_Q38_BANK_PROTOCOL;
	if (!Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return false;
	bankOpen=true; // A successful BEGIN reply may still describe a failed active session.
	return session.result.state==TT_RESIDENT_SESSION_READY && session.result.active
		&& session.result.host_ready && !session.result.cold && !session.result.uploaded_pages;
}

static bool Load(int fd,PioneerQwen35Gguf& source,const PioneerQwen35PackedPlan& plan,
	D::WorkPrepare& prepare,bool& spanMayBeOpen,bool& workPrepareAttempted)
{
	auto packet=std::make_unique<tt_q38_span_upload>();
	unsigned spans=0;
	uint64_t uploaded=0;
	auto upload=[&](unsigned bank,unsigned offset,unsigned bytes,const uint8_t*) {
		spanMayBeOpen=true; // The ioctl may create or advance the span before reporting an error.
		packet->version=TT_Q38_SPAN_PUBLIC_VERSION;
		packet->bank=bank;
		packet->offset=offset;
		packet->bytes=bytes;
		packet->result={};
		if (!Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,packet.get(),sizeof(*packet))) return false;
		const auto& r=packet->result;
		if (packet->version!=TT_Q38_SPAN_PUBLIC_VERSION || packet->bank!=bank
			|| packet->offset!=offset || packet->bytes!=bytes || packet->reserved
			|| r.version!=1 || r.state!=TT_RUNTIME_PASSED || r.bytes!=bytes
			|| r.pages!=bytes/D::LoadPageBytes || r.nonce!=spans+1
			|| !r.started || !r.completed || !r.held || r.host_exact
			|| r.matched_words || r.recovery_required || r.reserved) {
			fprintf(stderr,"P100A_Q35 upload failed span=%u bank=%u offset=%u state=%u cold=%u\n",
				spans+1,bank,offset,r.state,r.recovery_required);
			return false;
		}
		if (++spans%128==0) fprintf(stderr,"P100A_Q35 uploaded_spans=%u bytes=%llu\n",
			spans,static_cast<unsigned long long>(uploaded+bytes));
		return true;
	};
	if (!D::UploadModel(source,plan,prepare.manifest,reinterpret_cast<uint8_t*>(packet->data),
		sizeof(packet->data),upload,uploaded)) return false;
	tt_q38_span_end end={}; end.version=TT_Q38_SPAN_PUBLIC_VERSION;
	if (!Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end)) || !end.closed || end.recovery_required)
		return false;
	spanMayBeOpen=false;
	workPrepareAttempted=true; // PREPARE can bind an owner even when its reply is lost.
	if (!Call(fd,TT_PCI_PROBE_Q35_WORK_PREPARE,&prepare,sizeof(prepare))) return false;
	printf("P100A_Q35 resident_spans=%u resident_bytes=%llu\n",spans,
		static_cast<unsigned long long>(uploaded));
	return true;
}

static bool Generate(int fd,const Vocabulary& vocab,D::WorkStart& request,unsigned context,
	std::string& output,bool& held)
{
	if (!Call(fd,TT_PCI_PROBE_Q35_WORK_START,&request,sizeof(request))) return false;
	held=true; // END must run even when polling reports a recoverable Work fault.
	auto poll=std::make_unique<D::WorkPoll>();
	unsigned committed=0, generated=0;
	for (;;) {
		poll->version=D::WorkVersion;
		poll->requestId=request.requestId;
		if (!Call(fd,TT_PCI_PROBE_Q35_WORK_POLL,poll.get(),sizeof(*poll))) return false;
		const auto& p=poll->progress;
		if (p.committed!=committed || p.generated!=generated) {
			committed=p.committed;
			generated=p.generated;
			fprintf(stderr,"P100A_Q35 run=%u committed=%u prefilled=%u generated=%u pc=%#x\n",
				request.requestId,committed,p.prefilled,generated,poll->pc);
		}
		if (poll->state==D::WorkComplete) break;
		if (poll->state!=D::WorkRunning) {
			if (poll->state==D::WorkRecoverableFault)
				fprintf(stderr,"P100A_Q35 terminal Work fault passed the first safe-idle proof; STOP will repeat it\n");
			if (poll->state==D::WorkRecoveryPending)
				fprintf(stderr,"P100A_Q35 terminal Work fault proof pending reason=%u detail=%#x; requesting STOP reproof\n",
					poll->recoveryProofReason,poll->recoveryProofDetail);
			fprintf(stderr,"P100A_Q35 unexpected terminal run=%u state=%u commits=%u pc=%#x "
				"progress_magic=%#x prefilled=%u root_trace=%#x fault_marker=%#x "
				"dispatcher_stage=%#x dispatcher_detail=%#x dispatcher_context0=%#x "
				"dispatcher_context1=%#x scale_before_clear=%#x scale_after_clear=%#x "
				"scale_after_stage_input=%#x scale_at_stageband_entry=%#x "
				"scale_after_initial_validation=%#x scale_after_fragments=%#x "
				"scale_after_state_validation=%#x scale_before_batch_object=%#x "
				"scale_after_batch_object=%#x scale_after_batch_build=%#x "
				"scale_after_batch_fields=%#x scale_after_root_job=%#x "
				"scale_after_worker_table=%#x scale_before_dispatch=%#x "
				"scale_before_validation=%#x recovery_proof=%u recovery_detail=%#x\n",
				request.requestId,poll->state,committed,poll->pc,p.magic,p.prefilled,
				p.reserved[0],p.reserved[1],p.reserved[2],p.reserved[3],p.reserved[4],p.reserved[5],
				poll->generatedIds[0],poll->generatedIds[1],poll->generatedIds[2],poll->generatedIds[3],
				poll->generatedIds[4],poll->generatedIds[5],poll->generatedIds[6],poll->generatedIds[7],
				poll->generatedIds[8],poll->generatedIds[9],poll->generatedIds[10],poll->generatedIds[11],
				poll->generatedIds[12],poll->generatedIds[13],poll->generatedIds[14],
				poll->recoveryProofReason,poll->recoveryProofDetail);
			fprintf(stderr,"P100A_Q35 root_noc_snapshot_valid=%u command=",
				poll->faultNocValid);
			for (unsigned i=0;i<10;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultNocCommand[i]);
			fprintf(stderr," counters=");
			for (unsigned i=0;i<8;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultNocCounters[i]);
			fprintf(stderr," counter_delta=");
			for (unsigned i=0;i<8;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultNocCounterDelta[i]);
			fprintf(stderr,"\n");
			fprintf(stderr,"P100A_Q35 worker119_snapshot_valid=%u pc=%#x,%#x status=",
				poll->faultPeer119Valid,poll->faultPeer119Pc[0],poll->faultPeer119Pc[1]);
			for (unsigned i=0;i<16;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultPeer119Status[i]);
			fprintf(stderr,"\n");
			fprintf(stderr,"P100A_Q35 root_job_snapshot_valid=%u words=",poll->faultRootJobValid);
			for (unsigned i=0;i<16;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultRootJob[i]);
			fprintf(stderr,"\n");
			fprintf(stderr,"P100A_Q35 worker119_job_snapshot_valid=%u words=",poll->faultPeer119JobValid);
			for (unsigned i=0;i<16;i++) fprintf(stderr,"%s%#x",i?",":"",poll->faultPeer119Job[i]);
			fprintf(stderr,"\n");
			return false;
		}
		snooze(10000);
	}
	if (!D::ValidWorkResult(*poll,request.promptCount,request.maxOutput,context,poll->progress.generated)
		|| poll->prefillObservedMicros<=poll->startMicros
		|| poll->completeObservedMicros<=poll->prefillObservedMicros
		|| !vocab.Detokenize(*poll,output)) return false;
	const auto& p=poll->progress;
	const double promptSec=(poll->prefillObservedMicros-poll->startMicros)*1e-6;
	const double decodeSec=(poll->completeObservedMicros-poll->prefillObservedMicros)*1e-6;
	printf("P100A_Q35 warm_run=%u prompt_tokens=%u prompt_seconds=%.6f output_tokens=%u "
		"decode_seconds=%.6f decode_tokens_per_second=%.6f prompt_cycles=%llu decode_cycles=%llu "
		"full_core_waves=%u projection_waves=%u activation_pack_cycles_256=%u "
		"projection_setup_cycles_256=%u matrix_dispatch_cycles_256=%u\n",
		request.requestId,p.prefilled,promptSec,p.generated,decodeSec,p.generated/decodeSec,
		(static_cast<unsigned long long>(p.promptCyclesHigh)<<32)|p.promptCyclesLow,
		(static_cast<unsigned long long>(p.decodeCyclesHigh)<<32)|p.decodeCyclesLow,
		p.reserved[D::ProgressFullCoreWavesSlot],p.reserved[D::ProgressProjectionWavesSlot],
		p.reserved[D::ProgressActivationPackTicksSlot],p.reserved[D::ProgressProjectionSetupTicksSlot],
		p.reserved[D::ProgressMatrixDispatchTicksSlot]);
	printf("P100A_Q35 generated_ids=");
	for (unsigned i=0; i<p.generated; i++) printf("%s%u",i?",":"",poll->generatedIds[i]);
	printf("\n");
	printf("P100A_Q35 output_begin run=%u\n",request.requestId);
	fwrite(output.data(),1,output.size(),stdout);
	printf("\nP100A_Q35 output_end run=%u\n",request.requestId);
	return true;
}

static bool End(int fd,bool bankOpen,bool spanMayBeOpen,bool workStopNeeded)
{
	bool okay=true;
	bool workStopped=!workStopNeeded;
	if (spanMayBeOpen) {
		tt_q38_span_end span={}; span.version=TT_Q38_SPAN_PUBLIC_VERSION;
		const bool spanEnded=Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&span,sizeof(span))
			&& span.closed && !span.recovery_required;
		if (!spanEnded) fprintf(stderr,"P100A_Q35 span recovery failed closed=%u recovery=%u\n",
			span.closed,span.recovery_required);
		okay=spanEnded && okay;
	}
	if (workStopNeeded) {
		D::WorkStop stop={}; stop.version=D::WorkVersion;
		bool stopped=false;
		unsigned retries=0;
		for (;;) {
			if (!Call(fd,TT_PCI_PROBE_Q35_WORK_STOP,&stop,sizeof(stop))) break;
			if (stop.restored) { stopped=true; break; }
			if (!stop.retryable) break;
			if (!retries) fprintf(stderr,
				"P100A_Q35 STOP proof pending reason=%u detail=%#x; retrying read-only proof with the driver session held\n",
				stop.recoveryProofReason,stop.recoveryProofDetail);
			else if (retries%20==0) fprintf(stderr,
				"P100A_Q35 STOP proof still pending retry=%u reason=%u detail=%#x\n",
				retries,stop.recoveryProofReason,stop.recoveryProofDetail);
			retries++;
			snooze(100000);
			stop={}; stop.version=D::WorkVersion;
		}
		if (!stopped) fprintf(stderr,"P100A_Q35 STOP recovery failed proof=%u detail=%#x retryable=%u\n",
			stop.recoveryProofReason,stop.recoveryProofDetail,stop.retryable);
		okay=stopped && okay;
		workStopped=stopped;
	}
	tt_q38_bank_session session={}; session.version=TT_Q38_BANK_PROTOCOL;
	const bool bankEnded=!bankOpen || (workStopped && Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))
		&& session.result.state==TT_RESIDENT_SESSION_CLOSED && !session.result.active
		&& session.result.pci_restored && session.result.host_released && !session.result.cold);
	if (bankOpen) printf("P100A_Q35 unload_clean=%u state=%u active=%u cold=%u\n",
		unsigned(bankEnded),session.result.state,session.result.active,session.result.cold);
	okay=bankEnded && okay;
	return okay;
}

int main(int argc,char** argv)
{
	setvbuf(stdout,nullptr,_IOLBF,0);
	const bool vocabOnly=argc==5 && !strcmp(argv[1],"--check-vocab");
	if (!vocabOnly && (argc<5 || argc>6)) {
		fprintf(stderr,"usage: %s GGUF ABSOLUTE_VOCAB_LIBRARY PROMPT OUTPUT_TOKENS [CONTEXT]\n",argv[0]);
		fprintf(stderr,"       %s --check-vocab GGUF ABSOLUTE_VOCAB_LIBRARY PROMPT\n",argv[0]);
		return 2;
	}
	const unsigned context=argc==6?Parse(argv[5],D::MaxPositions):128;
	const unsigned outputLimit=vocabOnly?1:Parse(argv[4],context);
	if (!context || !outputLimit) return 2;
	const unsigned shift=unsigned(vocabOnly);
	const int sourceFD=open(argv[1+shift],O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
	if (sourceFD<0) { perror("Open pinned GGUF"); return 1; }
	std::unique_ptr<FILE,decltype(&fclose)> source(fdopen(sourceFD,"rb"),&fclose);
	if (!source) { close(sourceFD); return 1; }
	const auto authStart=system_time();
	PioneerQwen35Gguf packed;
	PioneerModelWeightCache::Digest digest{};
	std::string error;
	if (!packed.Open(source.get())
		|| !PioneerModelWeightCache::HashFile(sourceFD,packed.FullBytes,digest,error)
		|| PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
		|| !packed.Stable()) {
		fprintf(stderr,"Pinned GGUF authentication failed. %s\n",error.c_str());
		return 1;
	}
	printf("P100A_Q35 authentication_seconds=%.6f\n",(system_time()-authStart)*1e-6);
	const auto vocabStart=system_time();
	Vocabulary vocab;
	auto request=std::make_unique<D::WorkStart>();
	request->version=D::WorkVersion;
	request->requestId=1;
	request->maxOutput=outputLimit;
	if (!vocab.Open(argv[2+shift],source.get()) || !packed.Stable()
		|| !vocab.Tokenize(argv[3+shift],*request,context)
		|| !D::ValidWorkStart(*request,context,1)) {
		fprintf(stderr,"Vocabulary or prompt validation failed.\n");
		return 1;
	}
	printf("P100A_Q35 vocab_and_tokenize_seconds=%.6f prompt_ids=",(system_time()-vocabStart)*1e-6);
	for (unsigned i=0; i<request->promptCount; i++) printf("%s%u",i?",":"",request->prompt[i]);
	printf("\n");
	if (vocabOnly) return 0;
	PioneerQwen35PackedPlan plan;
	auto prepare=std::make_unique<D::WorkPrepare>();
	prepare->version=D::WorkVersion;
	if (!plan.Build(packed,context,true) || !D::BuildDeviceLayout(packed,plan,context,prepare->manifest)
		|| !D::ResolveRope(prepare->manifest,plan,prepare->rope)) return 1;
	const int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (fd<0) { perror("Open Pioneer Work driver"); return 1; }
	const auto loadStart=system_time();
	bool bankOpen=false,spanMayBeOpen=false,workPrepareAttempted=false;
	if (!Begin(fd,bankOpen) || !Load(fd,packed,plan,*prepare,spanMayBeOpen,workPrepareAttempted)) {
		const bool cleaned=End(fd,bankOpen,spanMayBeOpen,workPrepareAttempted);
		const bool closed=close(fd)==0;
		fprintf(stderr,"P100A_Q35 model load failed; explicit cleanup=%u close=%u\n",
			unsigned(cleaned),unsigned(closed));
		return 1;
	}
	printf("P100A_Q35 model_load_seconds=%.6f\n",(system_time()-loadStart)*1e-6);
	std::string first, second;
	bool held=false;
	const bool firstOkay=Generate(fd,vocab,*request,context,first,held);
	request->requestId=2;
	const bool secondOkay=firstOkay && Generate(fd,vocab,*request,context,second,held);
	const bool ended=End(fd,bankOpen,spanMayBeOpen,workPrepareAttempted || held);
	const bool closed=close(fd)==0;
	return firstOkay && secondOkay && first==second && ended && closed?0:1;
}
