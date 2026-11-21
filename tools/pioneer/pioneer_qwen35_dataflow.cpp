/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <OS.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowModelProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35Preload.h"
#include "PioneerQwen35RootImageLayout.h"
#include "llama.h"

namespace D=PioneerQwen35;

class Vocabulary {
	void* fLibrary=nullptr;
	llama_model* fModel=nullptr;
	const llama_vocab* fVocab=nullptr;
	decltype(&llama_model_free) fFreeModel=nullptr;
	decltype(&llama_tokenize) fTokenize=nullptr;
	decltype(&llama_detokenize) fDetokenize=nullptr;

	template<class Function>
	Function Resolve(const char* name) const
	{
		return reinterpret_cast<Function>(dlsym(fLibrary,name));
	}

public:
	~Vocabulary()
	{
		if (fModel) fFreeModel(fModel);
		if (fLibrary) dlclose(fLibrary);
	}

	bool Open(const char* library,const char* modelPath,FILE* source)
	{
		if (!library || library[0]!='/' || !modelPath || modelPath[0]!='/'
			|| fseeko(source,0,SEEK_SET)) return false;
		fLibrary=dlopen(library,RTLD_NOW|RTLD_LOCAL);
		if (!fLibrary) {
			fprintf(stderr,"Vocabulary library: %s\n",dlerror());
			return false;
		}
		auto defaults=Resolve<decltype(&llama_model_default_params)>(
			"llama_model_default_params");
		auto load=Resolve<decltype(&llama_model_load_from_file_ptr)>(
			"llama_model_load_from_file_ptr");
		auto get=Resolve<decltype(&llama_model_get_vocab)>("llama_model_get_vocab");
		auto size=Resolve<decltype(&llama_vocab_n_tokens)>("llama_vocab_n_tokens");
		auto eos=Resolve<decltype(&llama_vocab_eos)>("llama_vocab_eos");
		auto text=Resolve<decltype(&llama_vocab_get_text)>("llama_vocab_get_text");
		fFreeModel=Resolve<decltype(fFreeModel)>("llama_model_free");
		fTokenize=Resolve<decltype(fTokenize)>("llama_tokenize");
		fDetokenize=Resolve<decltype(fDetokenize)>("llama_detokenize");
		if (!defaults || !load || !get || !size || !eos || !text || !fFreeModel
			|| !fTokenize || !fDetokenize) return false;
		auto params=defaults();
		ggml_backend_dev_t noDevices[]={nullptr};
		params.devices=noDevices;
		params.n_gpu_layers=0;
		params.load_mode=LLAMA_LOAD_MODE_NONE;
		params.vocab_only=true;
		fModel=load(source,params);
		if (!fModel) return false;
		fVocab=get(fModel);
		return fVocab && size(fVocab)==D::Vocabulary
			&& eos(fVocab)==D::EndOfTurn
			&& !strcmp(text(fVocab,D::EndOfText),"<|endoftext|>")
			&& !strcmp(text(fVocab,D::EndOfTurn),"<|im_end|>");
	}

	bool Tokenize(const char* prompt,unsigned maxContext,
		std::vector<unsigned>& tokens) const
	{
		const size_t bytes=strlen(prompt);
		if (!fVocab || !bytes || bytes>INT_MAX || !maxContext) return false;
		const bool chatml=bytes>=sizeof("<|im_start|>")-1
			&& !memcmp(prompt,"<|im_start|>",sizeof("<|im_start|>")-1);
		std::vector<llama_token> parsed(maxContext);
		const int count=fTokenize(fVocab,prompt,int(bytes),parsed.data(),
			parsed.size(),!chatml,chatml);
		if (count<=0 || unsigned(count)>maxContext) return false;
		tokens.resize(unsigned(count));
		for (int i=0;i<count;i++) {
			if (parsed[i]<0 || unsigned(parsed[i])>=D::Vocabulary) return false;
			tokens[unsigned(i)]=unsigned(parsed[i]);
		}
		return true;
	}

	bool Detokenize(const std::vector<unsigned>& tokens,std::string& output) const
	{
		if (!fVocab || tokens.empty()) return false;
		std::vector<llama_token> parsed(tokens.begin(),tokens.end());
		const int needed=fDetokenize(fVocab,parsed.data(),parsed.size(),nullptr,0,
			true,false);
		if (needed>=0 || needed==INT_MIN) return false;
		output.resize(unsigned(-needed));
		return fDetokenize(fVocab,parsed.data(),parsed.size(),output.data(),
			output.size(),true,false)==-needed && !output.empty();
	}
};

static bool Call(int fd,uint32 operation,void* data,size_t bytes)
{
	if (ioctl(fd,operation,data,bytes)==0) return true;
	fprintf(stderr,"P100A_Q35_DATAFLOW ioctl=%08x error=%s\n",
		operation,strerror(errno));
	return false;
}

static unsigned Parse(const char* text,unsigned maximum)
{
	if (!text || *text<'0' || *text>'9') return 0;
	errno=0;
	char* end=nullptr;
	const unsigned long value=strtoul(text,&end,10);
	return errno || *end || value>maximum?0:unsigned(value);
}

static bool Begin(int fd,bool& bankOpen)
{
	bankOpen=false;
	tt_qwen35_dataflow_support dataflow{};
	tt_q38_bank_support bank{};
	tt_q38_span_support span{};
	if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT,&dataflow,sizeof(dataflow))
		|| dataflow.version!=TT_QWEN35_DATAFLOW_ABI
		|| dataflow.driver!=TT_QWEN35_DATAFLOW_DRIVER
		|| dataflow.cores!=D::DataflowCoreCount
		|| (dataflow.capabilities
			& (TT_QWEN35_DATAFLOW_CAP_RESIDENT_SERVICE
				|TT_QWEN35_DATAFLOW_CAP_MANIFEST_BIND
				|TT_QWEN35_DATAFLOW_CAP_TOKEN_EMBEDDING
				|TT_QWEN35_DATAFLOW_CAP_DECODER_SEQUENCE
				|TT_QWEN35_DATAFLOW_CAP_GENERATION))
			!= (TT_QWEN35_DATAFLOW_CAP_RESIDENT_SERVICE
				|TT_QWEN35_DATAFLOW_CAP_MANIFEST_BIND
				|TT_QWEN35_DATAFLOW_CAP_TOKEN_EMBEDDING
				|TT_QWEN35_DATAFLOW_CAP_DECODER_SEQUENCE
				|TT_QWEN35_DATAFLOW_CAP_GENERATION)
		|| dataflow.projection_capabilities!=TT_QWEN35_DATAFLOW_PROJECTION_MATRIX
		|| dataflow.token_capabilities!=TT_QWEN35_DATAFLOW_TOKEN_EMBEDDING
		|| !Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&bank,sizeof(bank))
		|| bank.driver!=TT_Q38_BANK_DRIVER || bank.version!=TT_Q38_BANK_PROTOCOL
		|| bank.banks!=D::Banks || bank.page_bytes!=D::LoadPageBytes
		|| !bank.storage_only || bank.bytes_per_bank!=PioneerQwen35PackedPlan::Capacity
		|| !Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT,&span,sizeof(span))
		|| span.version!=TT_Q38_SPAN_PUBLIC_VERSION || span.banks!=D::Banks
		|| span.page_bytes!=D::LoadPageBytes || span.max_bytes!=D::LoadSpanBytes
		|| !span.before_cohort || !span.explicit_end || span.reserved) return false;

	tt_q38_bank_session session{};
	session.version=TT_Q38_BANK_PROTOCOL;
	if (!Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return false;
	bankOpen=true;
	return session.result.state==TT_RESIDENT_SESSION_READY
		&& session.result.active && session.result.host_ready
		&& !session.result.cold && !session.result.uploaded_pages;
}

static bool Upload(int fd,PioneerQwen35Gguf& source,
	const PioneerQwen35PackedPlan& plan,const D::DeviceLayout& manifest,
	bool& spanOpen,uint64_t& uploadedBytes)
{
	auto packet=std::make_unique<tt_q38_span_upload>();
	unsigned spans=0;
	auto upload=[&](unsigned bank,unsigned offset,unsigned bytes,const uint8_t*) {
		spanOpen=true;
		packet->version=TT_Q38_SPAN_PUBLIC_VERSION;
		packet->bank=bank;
		packet->offset=offset;
		packet->bytes=bytes;
		packet->reserved=0;
		packet->result={};
		if (!Call(fd,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,packet.get(),sizeof(*packet)))
			return false;
		const auto& result=packet->result;
		if (packet->version!=TT_Q38_SPAN_PUBLIC_VERSION || packet->bank!=bank
			|| packet->offset!=offset || packet->bytes!=bytes || packet->reserved
			|| result.version!=1 || result.state!=TT_RUNTIME_PASSED
			|| result.bytes!=bytes || result.pages!=bytes/D::LoadPageBytes
			|| result.nonce!=spans+1 || !result.started || !result.completed
			|| !result.held || result.host_exact || result.matched_words
			|| result.recovery_required || result.reserved) {
			fprintf(stderr,"P100A_Q35_DATAFLOW upload failed span=%u bank=%u "
				"offset=%u state=%u recovery=%u\n",spans+1,bank,offset,
				result.state,result.recovery_required);
			return false;
		}
		spans++;
		if (!(spans%128)) fprintf(stderr,
			"P100A_Q35_DATAFLOW uploaded_spans=%u bytes=%llu\n",spans,
			static_cast<unsigned long long>(uploadedBytes+bytes));
		return source.Stable();
	};

	D::Range rope{};
	if (!D::ResolveRope(manifest,plan,rope)
		|| !D::UploadModel(source,plan,manifest,
			reinterpret_cast<uint8_t*>(packet->data),sizeof(packet->data),
			upload,uploadedBytes)) return false;
	tt_q38_span_end end{};
	end.version=TT_Q38_SPAN_PUBLIC_VERSION;
	if (!Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))
		|| !end.closed || end.recovery_required) return false;
	spanOpen=false;
	return source.Stable();
}

static bool BindAndInstall(int fd,const D::DeviceLayout& manifest,
	const D::DataflowGraphWorkspace& graph,const D::Range& rope,
	bool& authorityBound,bool& installAttempted)
{
	auto bind=std::make_unique<tt_qwen35_dataflow_model_bind>();
	bind->version=TT_QWEN35_DATAFLOW_MODEL_ABI;
	bind->rope=rope;
	bind->manifest=manifest;
	bind->graphWorkspace=graph;
	if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_MODEL_BIND,bind.get(),sizeof(*bind))) {
		if (bind->reserved&TT_QWEN35_DATAFLOW_BIND_DIAG_ROUTE)
			fprintf(stderr,"P100A_Q35_DATAFLOW bind_route_failure stage=%u core_or_bank=%u\n",
				(bind->reserved>>8)&0xff,bind->reserved&0xff);
		else fprintf(stderr,"P100A_Q35_DATAFLOW bind_failure_stage=%u\n",
			bind->reserved);
		return false;
	}
	authorityBound=true;

	tt_qwen35_dataflow_request request{};
	request.version=TT_QWEN35_DATAFLOW_ABI;
	request.action=TT_QWEN35_DATAFLOW_ACTION_INSTALL;
	installAttempted=true;
	if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&request,sizeof(request)))
		return false;
	if (request.result==TT_QWEN35_DATAFLOW_RESULT_COMPLETE
		&& request.state==TT_QWEN35_DATAFLOW_STATE_READY
		&& !request.recovery_required && !request.reserved[0]
		&& !request.reserved[1] && !request.reserved[2])
		return true;
	fprintf(stderr,"P100A_Q35_DATAFLOW install_failed result=%u state=%u next_sequence=%u "
		"recovery=%u diag_stage=%u diag_core=%u worker_state=%u "
		"worker_failure=%u worker_phase=%u worker_route=0x%08x\n",
		request.result,request.state,
		request.next_sequence,request.recovery_required,request.reserved[0]>>24,
		request.reserved[0]&0x00ffffff,request.reserved[1]>>24,
		(request.reserved[1]>>16)&0xff,(request.reserved[1]>>8)&0xff,
		request.reserved[2]);
	return false;
}

static bool CloseRewrite(int fd,bool authorityBound,bool installAttempted,
	bool& serviceClosed)
{
	serviceClosed=!installAttempted;
	if (!installAttempted) return true;
	for (unsigned attempt=0;attempt<3;attempt++) {
		tt_qwen35_dataflow_request request{};
		request.version=TT_QWEN35_DATAFLOW_ABI;
		request.action=TT_QWEN35_DATAFLOW_ACTION_CLOSE;
		if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&request,sizeof(request)))
			break;
		serviceClosed=request.result==TT_QWEN35_DATAFLOW_RESULT_CLOSED
			&& request.state==TT_QWEN35_DATAFLOW_STATE_CLOSED
			&& !request.recovery_required;
		if (serviceClosed) return true;
		if (request.result || request.recovery_required || request.reserved[0]
			|| request.reserved[1] || request.reserved[2]) {
			fprintf(stderr,"P100A_Q35_DATAFLOW close_failed result=%u state=%u "
				"recovery=%u diag_stage=%u diag_core=%u diagnostic_state=%u "
				"diagnostic_failure=%u diagnostic_phase=%u resume_detail=0x%08x\n",
				request.result,request.state,request.recovery_required,
				request.reserved[0]>>24,request.reserved[0]&0x00ffffff,
				request.reserved[1]>>24,(request.reserved[1]>>16)&0xff,
				(request.reserved[1]>>8)&0xff,request.reserved[2]);
		}
		if (request.result!=TT_QWEN35_DATAFLOW_RESULT_RESUME_FAILED
			|| !request.recovery_required || attempt==2) break;
		fprintf(stderr,"P100A_Q35_DATAFLOW close_retry attempt=%u result=%u\n",
			attempt+1,request.result);
		snooze(10000);
	}
	tt_qwen35_dataflow_request request{};
	request.version=TT_QWEN35_DATAFLOW_ABI;
	request.action=TT_QWEN35_DATAFLOW_ACTION_QUERY;
	if (Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&request,sizeof(request)))
		serviceClosed=(request.state==TT_QWEN35_DATAFLOW_STATE_CLOSED
			&& !request.recovery_required)
			|| (authorityBound && request.state==TT_QWEN35_DATAFLOW_STATE_EMPTY
				&& !request.recovery_required);
	return serviceClosed;
}

static bool Generate(int fd,const Vocabulary& vocabulary,
	const std::vector<unsigned>& prompt,unsigned maxNewTokens,unsigned requestId,
	std::vector<unsigned>& tokens,std::string& text)
{
	tokens.assign(maxNewTokens,0);
	auto request=std::make_unique<tt_qwen35_dataflow_generate>();
	request->version=TT_QWEN35_DATAFLOW_MODEL_ABI;
	request->prompt_count=unsigned(prompt.size());
	request->max_new_tokens=maxNewTokens;
	request->input_slot=0;
	request->eos_token=D::EndOfTurn;
	request->prompt_tokens=uint64_t(reinterpret_cast<uintptr_t>(prompt.data()));
	request->output_tokens=uint64_t(reinterpret_cast<uintptr_t>(tokens.data()));
	if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_GENERATE,request.get(),
		sizeof(*request))) {
		fprintf(stderr,"P100A_Q35_DATAFLOW generate_request bytes=%zu version=%u "
			"prompt_count=%u max_new_tokens=%u input_slot=%u eos=%u "
			"result=%u stage=%u state=%u output_count=%u next_sequence=%u "
			"recovery=%u prompt_us=%llu decode_us=%llu "
			"prompt_ptr=0x%016llx output_ptr=0x%016llx\n",
			sizeof(*request),request->version,request->prompt_count,
			request->max_new_tokens,request->input_slot,request->eos_token,
			request->result,request->stage,request->state,request->output_count,
			request->next_sequence,request->recovery_required,
			static_cast<unsigned long long>(request->prompt_time_us),
			static_cast<unsigned long long>(request->decode_time_us),
			static_cast<unsigned long long>(request->prompt_tokens),
			static_cast<unsigned long long>(request->output_tokens));
		return false;
	}
		if (request->result!=TT_QWEN35_DATAFLOW_RESULT_COMPLETE
		|| request->state!=TT_QWEN35_DATAFLOW_STATE_READY
		|| request->recovery_required || request->stage
		|| request->output_count<2 || request->output_count>maxNewTokens
		|| !request->prompt_time_us || !request->decode_time_us) {
		fprintf(stderr,"P100A_Q35_DATAFLOW generation failed run=%u result=%u "
			"stage=%u state=%u recovery=%u output=%u prompt_us=%llu decode_us=%llu\n",
			requestId,request->result,request->stage,request->state,
			request->recovery_required,request->output_count,
			static_cast<unsigned long long>(request->prompt_time_us),
			static_cast<unsigned long long>(request->decode_time_us));
		return false;
	}
	tokens.resize(request->output_count);
	for (unsigned token:tokens)
		if (token>=D::Vocabulary) return false;
	if (!vocabulary.Detokenize(tokens,text)) return false;
	const double promptSeconds=double(request->prompt_time_us)*1e-6;
	const double decodeSeconds=double(request->decode_time_us)*1e-6;
	const double decodeRate=double(request->output_count-1)/decodeSeconds;
	printf("P100A_Q35_DATAFLOW warm_run=%u prompt_tokens=%u prompt_seconds=%.6f "
		"output_tokens=%u decode_seconds=%.6f decode_tokens_per_second=%.6f\n",
		requestId,unsigned(prompt.size()),promptSeconds,request->output_count,
		decodeSeconds,decodeRate);
	printf("P100A_Q35_DATAFLOW generated_ids=");
	for (unsigned i=0;i<tokens.size();i++)
		printf("%s%u",i?",":"",tokens[i]);
	printf("\nP100A_Q35_DATAFLOW output_begin run=%u\n",requestId);
	fwrite(text.data(),1,text.size(),stdout);
	printf("\nP100A_Q35_DATAFLOW output_end run=%u\n",requestId);
	return true;
}

static bool End(int fd,bool bankOpen,bool spanOpen,bool authorityBound,
	bool installAttempted)
{
	bool okay=true;
	if (spanOpen) {
		tt_q38_span_end span{};
		span.version=TT_Q38_SPAN_PUBLIC_VERSION;
		const bool ended=Call(fd,TT_PCI_PROBE_Q38_SPAN_END,&span,sizeof(span))
			&& span.closed && !span.recovery_required;
		fprintf(stderr,"P100A_Q35_DATAFLOW span_closed=%u recovery=%u\n",
			span.closed,span.recovery_required);
		okay=ended && okay;
	}
	bool serviceClosed=false;
	const bool closeOkay=CloseRewrite(fd,authorityBound,installAttempted,
		serviceClosed);
	okay=closeOkay && okay;
	tt_q38_bank_session session{};
	session.version=TT_Q38_BANK_PROTOCOL;
	const bool bankEnded=!bankOpen || (okay
		&& Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))
		&& session.result.state==TT_RESIDENT_SESSION_CLOSED
		&& !session.result.active && session.result.pci_restored
		&& session.result.host_released && !session.result.cold);
	if (bankOpen) printf("P100A_Q35_DATAFLOW unload_clean=%u state=%u active=%u "
		"cold=%u service_closed=%u\n",unsigned(bankEnded),
		session.result.state,session.result.active,session.result.cold,
		unsigned(serviceClosed));
	return bankEnded && okay;
}

int main(int argc,char** argv)
{
	setvbuf(stdout,nullptr,_IOLBF,0);
	if (argc<5 || argc>6) {
		fprintf(stderr,"usage: %s GGUF ABSOLUTE_VOCAB_LIBRARY PROMPT "
			"MAX_NEW_TOKENS [CONTEXT]\n",argv[0]);
		return 2;
	}
	const unsigned context=argc==6?Parse(argv[5],D::MaxPositions):128;
	const unsigned maxNewTokens=Parse(argv[4],context);
	if (!context || !maxNewTokens) return 2;
	const int sourceFD=open(argv[1],O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
	if (sourceFD<0) { perror("Open pinned GGUF"); return 1; }
	std::unique_ptr<FILE,decltype(&fclose)> source(fdopen(sourceFD,"rb"),&fclose);
	if (!source) { close(sourceFD); return 1; }
	const auto authStart=system_time();
	PioneerQwen35Gguf packed;
	PioneerModelWeightCache::Digest digest{};
	std::string error;
	if (!packed.Open(source.get())
		|| !PioneerModelWeightCache::HashFile(sourceFD,packed.FullBytes,digest,error)
		|| PioneerModelWeightCache::Hex(digest)
			!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
		|| !packed.Stable()) {
		fprintf(stderr,"Pinned GGUF authentication failed. %s\n",error.c_str());
		return 1;
	}
	printf("P100A_Q35_DATAFLOW authentication_seconds=%.6f\n",
		double(system_time()-authStart)*1e-6);

	const auto vocabStart=system_time();
	Vocabulary vocabulary;
	if (!vocabulary.Open(argv[2],argv[1],source.get())) {
		fprintf(stderr,"Pinned vocabulary could not be opened.\n");
		return 1;
	}
	std::vector<unsigned> prompt;
	if (!vocabulary.Tokenize(argv[3],context,prompt)
		|| prompt.size()>context || maxNewTokens>context-prompt.size()+1
		|| !packed.Stable()) {
		fprintf(stderr,"Prompt or context validation failed.\n");
		return 1;
	}
	printf("P100A_Q35_DATAFLOW vocabulary_seconds=%.6f prompt_ids=",
		double(system_time()-vocabStart)*1e-6);
	for (unsigned i=0;i<prompt.size();i++)
		printf("%s%u",i?",":"",prompt[i]);
	printf("\n");

	PioneerQwen35PackedPlan plan;
	auto manifest=std::make_unique<D::DeviceLayout>();
	auto graph=std::make_unique<D::DataflowGraphWorkspace>();
	D::Range rope{};
	if (!plan.Build(packed,context,true)
		|| !D::BuildDeviceLayout(packed,plan,context,*manifest)
		|| !D::DataflowGraphWorkspaceBuilder::Build(plan,*manifest,*graph)
		|| !D::ResolveRope(*manifest,plan,rope)) {
		fprintf(stderr,"Pinned Qwen3.5 dataflow layout could not be built.\n");
		return 1;
	}

	const int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	if (fd<0) { perror("Open Qwen35 dataflow driver"); return 1; }
	const auto loadStart=system_time();
	bool bankOpen=false,spanOpen=false,authorityBound=false,installAttempted=false;
	uint64_t uploadedBytes=0;
	bool loaded=Begin(fd,bankOpen);
	if (loaded) loaded=Upload(fd,packed,plan,*manifest,spanOpen,uploadedBytes);
	if (loaded) loaded=BindAndInstall(fd,*manifest,*graph,rope,
		authorityBound,installAttempted);
	if (!loaded) {
		fprintf(stderr,"P100A_Q35_DATAFLOW model setup failed bytes=%llu "
			"authority_bound=%u install_attempted=%u\n",
			static_cast<unsigned long long>(uploadedBytes),
			unsigned(authorityBound),unsigned(installAttempted));
		const bool cleaned=End(fd,bankOpen,spanOpen,authorityBound,
			installAttempted);
		const bool closed=close(fd)==0;
		fprintf(stderr,"P100A_Q35_DATAFLOW cleanup=%u fd_closed=%u\n",
			unsigned(cleaned),unsigned(closed));
		return 1;
	}
	printf("P100A_Q35_DATAFLOW model_load_seconds=%.6f resident_bytes=%llu\n",
		double(system_time()-loadStart)*1e-6,
		static_cast<unsigned long long>(uploadedBytes));

	std::vector<unsigned> firstTokens,secondTokens;
	std::string firstText,secondText;
	const bool firstOkay=Generate(fd,vocabulary,prompt,maxNewTokens,1,
		firstTokens,firstText);
	const bool secondOkay=firstOkay && Generate(fd,vocabulary,prompt,maxNewTokens,2,
		secondTokens,secondText);
	const bool exactPrefix=firstTokens.size()>=2 && secondTokens.size()>=2
		&& firstTokens[0]==11751 && firstTokens[1]==13
		&& secondTokens[0]==11751 && secondTokens[1]==13;
	const bool repeatable=firstTokens==secondTokens && firstText==secondText;
	if (!exactPrefix) fprintf(stderr,
		"P100A_Q35_DATAFLOW reference prefix mismatch expected=11751,13\n");
	if (!repeatable) fprintf(stderr,
		"P100A_Q35_DATAFLOW warm outputs differ\n");
	const bool ended=End(fd,bankOpen,spanOpen,authorityBound,installAttempted);
	const bool closed=close(fd)==0;
	return firstOkay && secondOkay && exactPrefix && repeatable && ended && closed?0:1;
}
