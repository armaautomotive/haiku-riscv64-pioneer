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
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerRecoveryProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35FleetInspectionProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35RiscResetProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35RoleStageProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionQualificationProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/BlackholeAsicReset.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "PioneerModelWeightCache.h"
#include "PioneerQwen35DataflowGraphWorkspace.h"
#include "PioneerQwen35DataflowRecoveryRetry.h"
#include "PioneerQwen35Preload.h"
#include "PioneerQwen35VerifiedSpan.h"
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

// PIONEER_Q35_BEGIN_CLIENT_BEGIN
static bool Begin(int fd,bool& bankOpen,bool preloadOnly=false)
{
	bankOpen=false;
	tt_qwen35_dataflow_support dataflow{};
	tt_q38_bank_support bank{};
	tt_q38_span_support span{};
	if (!preloadOnly && (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_SUPPORT,&dataflow,sizeof(dataflow))
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
		|| dataflow.token_capabilities!=TT_QWEN35_DATAFLOW_TOKEN_EMBEDDING)) return false;
	if (!Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&bank,sizeof(bank))
		|| bank.driver!=TT_Q38_BANK_DRIVER || bank.version!=TT_Q38_BANK_PROTOCOL
		|| bank.banks!=D::Banks || bank.page_bytes!=D::LoadPageBytes
		|| !bank.storage_only || bank.bytes_per_bank!=PioneerQwen35PackedPlan::Capacity
		|| !Call(fd,TT_PCI_PROBE_Q38_SPAN_SUPPORT,&span,sizeof(span))
		|| !D::VerifiedSpanSupport(span)) return false;

	tt_q38_bank_session session{};
	session.version=TT_Q38_BANK_PROTOCOL;
	if (!Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return false;
	bankOpen=true;
	return session.result.state==TT_RESIDENT_SESSION_READY
		&& session.result.active && session.result.host_ready
		&& !session.result.cold && !session.result.uploaded_pages;
}
// PIONEER_Q35_BEGIN_CLIENT_END

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
		if (!Call(fd,TT_PCI_PROBE_Q38_SPAN_UPLOAD,packet.get(),sizeof(*packet)))
			return false;
		const auto& result=packet->result;
		if (!D::VerifiedSpanReply(*packet,bank,offset,bytes,spans+1)) {
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

static void PrintOwnerDiagnostic(int fd,const char* action,
	const tt_qwen35_dataflow_request& request)
{
	const unsigned packed=request.reserved[0];
	const unsigned core=(packed>>8)&0xffffU;
	const unsigned worker=request.reserved[1];
	const unsigned recoveryCore=(worker>>11)&0x7fU;
	const unsigned recoveryDetail=worker&0x7ffU;
	fprintf(stderr,"P100A_Q35_DATAFLOW %s result=%u state=%u recovery=%u "
		"owner_result=%u owner_core=%s%u owner_operation=%u "
		"worker_state=%u worker_failure=%u worker_phase=%u "
		"recovery_audit_stage=%u recovery_audit_core=%s%u "
		"recovery_audit_detail=%u",
		action,request.result,request.state,request.recovery_required,
		packed>>24,core==0xffffU?"none/":"",core==0xffffU?0:core,
		packed&0xffU,(worker>>29)&0x7U,(worker>>25)&0xfU,
		(worker>>22)&0x7U,(worker>>18)&0xfU,
		recoveryCore==0x7fU?"none/":"",recoveryCore==0x7fU?0:recoveryCore,
		recoveryDetail);
	if ((recoveryDetail&0x400U) && ((worker>>18)&0xfU)==6
		&& ((worker>>25)&0xfU)==7) {
		fprintf(stderr," failed_read_stage=%u failed_read_index=%u "
			"failed_read_status=0x%08x",(recoveryDetail>>6)&0xfU,
			recoveryDetail&0x3fU,request.reserved[2]);
		if (((recoveryDetail>>6)&0xfU)==7) {
			tt_qwen35_dataflow_request expected{};
			expected.version=TT_QWEN35_DATAFLOW_ABI;
			expected.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_EXPECTED;
			const bool expectedRead=Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,
				&expected,sizeof(expected))
				&& expected.reserved[0]==request.reserved[0]
				&& expected.reserved[1]==request.reserved[1];
			if (expectedRead)
				fprintf(stderr," failed_read_expected=0x%08x",expected.reserved[2]);
			else fputs(" failed_read_expected=unavailable",stderr);
			tt_qwen35_dataflow_request noc{};
			noc.version=TT_QWEN35_DATAFLOW_ABI;
			noc.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_NOC;
			const bool nocRead=Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,
				&noc,sizeof(noc))
				&& noc.reserved[0]==request.reserved[0]
				&& noc.state==request.state && noc.result==request.result
				&& noc.next_sequence==request.next_sequence
				&& noc.recovery_required==request.recovery_required;
			if (nocRead)
				fprintf(stderr," failed_read_counter_match_mask=0x%02x "
					"failed_read_initiator_busy_mask=0x%x "
					"failed_read_outstanding_id_mask=0x%08x",
					noc.reserved[1]&0xffU,(noc.reserved[1]>>8)&0xfU,
					noc.reserved[2]);
			else fputs(" failed_read_noc_snapshot=unavailable",stderr);
			tt_qwen35_dataflow_request readRequest{};
			readRequest.version=TT_QWEN35_DATAFLOW_ABI;
			readRequest.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_READ_REQUEST;
			const bool readRequestOkay=Call(fd,
				TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&readRequest,
				sizeof(readRequest))
				&& readRequest.reserved[0]==request.reserved[0]
				&& readRequest.state==request.state
				&& readRequest.result==request.result;
			if (readRequestOkay) {
				fprintf(stderr," failed_read_address=0x%08x "
					"failed_read_route=0x%03x failed_read_bytes=%u\n",
					readRequest.reserved[2],readRequest.reserved[1]&0xfffU,
					readRequest.reserved[1]>>12);
			} else fputs(" failed_read_request_identity=unavailable\n",stderr);
		} else fputc('\n',stderr);
	} else fprintf(stderr," pending_sequence=%u\n",request.reserved[2]);
}

static bool RecoverRewrite(int fd,bool installAttempted)
{
	if (!installAttempted) return true;
	tt_qwen35_dataflow_request query{};
	query.version=TT_QWEN35_DATAFLOW_ABI;
	query.action=TT_QWEN35_DATAFLOW_ACTION_QUERY;
	if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&query,sizeof(query))) {
		fputs("P100A_Q35_DATAFLOW recover_query_failed\n",stderr);
		return false;
	}
	if (query.state!=TT_QWEN35_DATAFLOW_STATE_FAULTED
		|| !query.recovery_required) return true;
	PrintOwnerDiagnostic(fd,"recover_before_close",query);
	tt_qwen35_dataflow_request graph{};
	graph.version=TT_QWEN35_DATAFLOW_ABI;
	graph.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_FAILURE;
	const bool graphRetry=Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&graph,sizeof(graph))
		&& (graph.reserved[0]&0x80000000U);
	struct Retry {
		int fd;
		bool graphRetry;
		D::DataflowRecoveryAttemptResult Attempt(unsigned attempt) {
			tt_qwen35_dataflow_request request{};
			request.version=TT_QWEN35_DATAFLOW_ABI;
			request.action=TT_QWEN35_DATAFLOW_ACTION_RECOVER;
			if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&request,sizeof(request))) {
				fputs("P100A_Q35_DATAFLOW recover_request_failed\n",stderr);
				return D::DataflowRecoveryAttemptStop;
			}
			if (request.result==TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED
				|| request.result==TT_QWEN35_DATAFLOW_RESULT_RESTORE_FAILED
				|| request.result==TT_QWEN35_DATAFLOW_RESULT_RESUME_FAILED) {
				fprintf(stderr,"P100A_Q35_DATAFLOW recover_close result=%u state=%u "
					"recovery=%u diag_stage=%u diag_core=%u diagnostic_state=%u "
					"diagnostic_failure=%u diagnostic_phase=%u resume_detail=0x%08x\n",
					request.result,request.state,request.recovery_required,
					request.reserved[0]>>24,request.reserved[0]&0x00ffffff,
					request.reserved[1]>>24,(request.reserved[1]>>16)&0xff,
					(request.reserved[1]>>8)&0xff,request.reserved[2]);
			} else PrintOwnerDiagnostic(fd,"recover_result",request);
			if (!request.recovery_required
				&& ((request.result==TT_QWEN35_DATAFLOW_RESULT_RECOVERED
						&& request.state==TT_QWEN35_DATAFLOW_STATE_READY)
					|| (request.result==TT_QWEN35_DATAFLOW_RESULT_CLOSED
						&& request.state==TT_QWEN35_DATAFLOW_STATE_CLOSED)))
				return D::DataflowRecoveryAttemptComplete;
			if (graphRetry && request.result==TT_QWEN35_DATAFLOW_RESULT_RECOVERY_PENDING
				&& request.state==TT_QWEN35_DATAFLOW_STATE_FAULTED
				&& request.recovery_required) {
				fprintf(stderr,"P100A_Q35_DATAFLOW recover_pending attempt=%u\n",attempt+1);
				return D::DataflowRecoveryAttemptPending;
			}
			return D::DataflowRecoveryAttemptStop;
		}
		bool Delay() { return snooze(1000000)==B_OK; }
	} retry{fd,graphRetry};
	return D::DataflowRecoveryRetry(retry,4);
}

// PIONEER_QWEN35_LIVE_RESET_CLIENT_BEGIN
static bool ResetStoppedRewrite(int fd,bool force,bool qualification=false)
{
	tt_pioneer_driver_reload before{};before.version=1;
	if (!Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&before,sizeof(before))
		|| before.driver!=TT_QWEN35_DATAFLOW_DRIVER || !before.build_tag || !before.generation
		|| (qualification && before.build_tag!=UINT64_C(0x2026112500030024)
			&& before.build_tag!=UINT64_C(0x2026112300030001)
			&& before.build_tag!=UINT64_C(0x2026112300030002)
			&& before.build_tag!=UINT64_C(0x2026112400030022))
		|| (!force && !before.cold) || before.quiesced || !before.active_owner || before.open_count!=1) return false;
	auto verifyIdle=[&](uint64_t expectedGeneration,bool exact) {
		tt_pioneer_driver_reload after{};after.version=1;
		return Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&after,sizeof(after))
			&& after.driver==TT_QWEN35_DATAFLOW_DRIVER && after.build_tag==before.build_tag
			&& after.generation && after.generation!=before.generation
			&& (!exact || after.generation==expectedGeneration)
			&& !after.cold && !after.quiesced && !after.active_owner && after.open_count==1;
	};
	for (unsigned attempt=0;attempt<4;attempt++) {
		tt_pioneer_recovery_run reset{};reset.version=1;
		reset.expected_tag=before.build_tag;reset.expected_generation=before.generation;
		// A lost ioctl reply may follow successful reset completion. Recheck
		// the guard state; never issue a second request on an unknown reply.
		const uint32_t operation=qualification && !attempt?TT_PCI_PROBE_RECOVERY_QUALIFY
			:force && !attempt?TT_PCI_PROBE_RECOVERY_FORCE:TT_PCI_PROBE_RECOVERY_RUN;
		if (!Call(fd,operation,&reset,sizeof(reset))) {
			fprintf(stderr,"P100A_Q35_RECOVERY ioctl_failed attempt=%u operation=0x%08x\n",
				attempt+1,operation);
			return verifyIdle(0,false);
		}
		fprintf(stderr,"P100A_Q35_RECOVERY attempt=%u stage=%u detail=0x%08x "
			"status=%ld generation_or_aer=0x%016llx pre_pcie=0x%08x\n",attempt+1,reset.stage,reset.detail,
			static_cast<long>(reset.operation_status),
			static_cast<unsigned long long>(reset.new_generation),reset.output_reserved);
		if (reset.operation_status==B_OK)
			return reset.stage==PIONEER_RECOVERY_STAGE_COMPLETE && reset.build_tag==before.build_tag
				&& reset.new_generation && verifyIdle(reset.new_generation,true);
		const bool pending=reset.operation_status==B_DEV_NOT_READY
			&& ((reset.stage==PIONEER_RECOVERY_STAGE_DISABLE_MASTER
					&& reset.detail==PIONEER_RECOVERY_DETAIL_PREPARE_WAIT)
				|| (reset.stage==PIONEER_RECOVERY_STAGE_ASIC_RESET && reset.detail==BLACKHOLE_ASIC_RESET_TIMEOUT)
				|| (reset.stage==PIONEER_RECOVERY_STAGE_FIRMWARE
					&& reset.detail==PIONEER_RECOVERY_DETAIL_FIRMWARE_WAIT)
				|| (reset.stage==PIONEER_RECOVERY_STAGE_MAPPING
					&& reset.detail==PIONEER_RECOVERY_DETAIL_MAPPING_WAIT)
				|| reset.stage==PIONEER_RECOVERY_STAGE_OWNER_RELEASE);
		if (!pending || attempt==3 || snooze(1000000)!=B_OK) return false;
	}
	return false;
}
// PIONEER_QWEN35_LIVE_RESET_CLIENT_END

// PIONEER_QWEN35_CLOSE_CLIENT_BEGIN
static bool CloseRewrite(int fd,bool authorityBound,bool installAttempted,
	bool& serviceClosed,bool& resetCompleted)
{
	serviceClosed=!installAttempted;
	resetCompleted=false;
	if (!installAttempted) return true;
	for (unsigned attempt=0;attempt<3;attempt++) {
		tt_qwen35_dataflow_request request{};
		request.version=TT_QWEN35_DATAFLOW_ABI;
		request.action=TT_QWEN35_DATAFLOW_ACTION_CLOSE;
		if (!Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&request,sizeof(request))) {
			// A cold latch refuses ordinary close, and the reply may also be
			// lost after a fault. The driver's explicit recovery admission
			// checks the live owner and fault before touching the device.
			resetCompleted=ResetStoppedRewrite(fd,true);
			serviceClosed=resetCompleted;
			return resetCompleted;
		}
		serviceClosed=request.result==TT_QWEN35_DATAFLOW_RESULT_CLOSED
			&& request.state==TT_QWEN35_DATAFLOW_STATE_CLOSED
			&& !request.recovery_required;
		if (serviceClosed) return true;
		if (request.result==TT_QWEN35_DATAFLOW_RESULT_RESET_REQUIRED
			&& request.state==TT_QWEN35_DATAFLOW_STATE_FAULTED && request.recovery_required) {
			resetCompleted=ResetStoppedRewrite(fd,false);
			serviceClosed=resetCompleted;
			return resetCompleted;
		}
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
		if (request.result==TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED
			&& request.recovery_required && attempt<2) {
			if (RecoverRewrite(fd,installAttempted)) {
				fprintf(stderr,"P100A_Q35_DATAFLOW close_retry_after_recovery "
					"attempt=%u\n",attempt+1);
				continue;
			}
			if (request.state==TT_QWEN35_DATAFLOW_STATE_FAULTED) {
				resetCompleted=ResetStoppedRewrite(fd,true);
				serviceClosed=resetCompleted;
				return resetCompleted;
			}
			break;
		}
		if (request.state==TT_QWEN35_DATAFLOW_STATE_FAULTED && request.recovery_required
			&& ((request.result==TT_QWEN35_DATAFLOW_RESULT_STOP_AUDIT_FAILED && attempt==2)
				|| request.result==TT_QWEN35_DATAFLOW_RESULT_RESTORE_FAILED
				|| request.result==TT_QWEN35_DATAFLOW_RESULT_RECOVERY_PENDING
				|| (request.result==TT_QWEN35_DATAFLOW_RESULT_RESUME_FAILED && attempt==2))) {
			resetCompleted=ResetStoppedRewrite(fd,true);
			serviceClosed=resetCompleted;
			return resetCompleted;
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
// PIONEER_QWEN35_CLOSE_CLIENT_END

static bool Generate(int fd,const Vocabulary& vocabulary,
	const std::vector<unsigned>& prompt,unsigned maxNewTokens,unsigned requestId,
	std::vector<unsigned>& tokens,std::string& text,bool diagnostic)
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
		if ((request->stage&0xff00U)==0x5200U && request->recovery_required) {
			tt_qwen35_dataflow_request graph{};
			graph.version=TT_QWEN35_DATAFLOW_ABI;
			graph.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_FAILURE;
			if (Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&graph,sizeof(graph))
				&& (graph.reserved[0]&0x80000000U)) {
				const bool terminalDetail=(graph.reserved[1]&0x80000000U)!=0;
				fprintf(stderr,"P100A_Q35_DATAFLOW graph_worker core=%u state=%u "
					"failure=%u phase=%u graph_detail_valid=%u graph_fault=%u graph_steps=%u "
					"graph_failure_core=%u graph_failure_status=0x%08x\n",
					graph.reserved[0]&0xffU,(graph.reserved[0]>>8)&0xfU,
					(graph.reserved[0]>>12)&0xffU,(graph.reserved[0]>>20)&0xfU,
					unsigned(terminalDetail),terminalDetail?graph.reserved[1]&0xfU:0,
					terminalDetail?(graph.reserved[1]>>4)&0x3ffU:0,
					terminalDetail?(graph.reserved[1]>>14)&0x7fU:0,
					terminalDetail?graph.reserved[2]:0);
				if ((graph.reserved[1]&0xc0000000U)==0x40000000U)
					fprintf(stderr,"P100A_Q35_DATAFLOW graph_progress step=%u phase=%u\n",
						(graph.reserved[1]>>4)&0x3ffU,graph.reserved[1]&0xfU);
				if ((graph.reserved[1]&0xc0000000U)==0x40000000U
					&& (graph.reserved[2]&0xffffff00U)==0x40000000U)
					fprintf(stderr,"P100A_Q35_DATAFLOW conv_progress group=%u stage=%u\n",
						(graph.reserved[2]>>5)&7U,graph.reserved[2]&31U);
				if (terminalDetail && (graph.reserved[1]&0xfU)==1 && (graph.reserved[2]&0x80000000U)) {
					const unsigned detail=graph.reserved[2];
					fprintf(stderr,"P100A_Q35_DATAFLOW graph_fetch stage=%u noc_stage=%u "
						"noc_index=%u counter_match_mask=0x%02x initiator_busy_mask=0x%x\n",
						detail&0xfU,(detail>>4)&0xfU,(detail>>8)&0xffU,
						(detail>>16)&0xffU,(detail>>24)&0xfU);
				}
				if (terminalDetail && (graph.reserved[1]&0xfU)==3 && (graph.reserved[2]&0x80000000U)) {
					const unsigned detail=graph.reserved[2];
					fprintf(stderr,"P100A_Q35_DATAFLOW rms_worker stage=%u noc_stage=%u "
						"noc_index=%u counter_match_mask=0x%02x initiator_busy_mask=0x%x\n",
						detail&0xfU,(detail>>4)&0xfU,(detail>>8)&0xffU,
						(detail>>16)&0xffU,(detail>>24)&0xfU);
				}
			} else fputs("P100A_Q35_DATAFLOW graph_worker unavailable\n",stderr);
		}
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
	if (diagnostic) {
		tt_qwen35_dataflow_request result{};
		result.version=TT_QWEN35_DATAFLOW_ABI;
		result.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_RESULT;
		if (Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&result,sizeof(result))
			&& (result.reserved[0]&0x80000000U)) {
			fprintf(stderr,"P100A_Q35_DATAFLOW graph_result token=%u score_bits=0x%08x sequence=%u\n",
				result.reserved[0]&0x7fffffffU,result.reserved[1],result.reserved[2]);
			for (unsigned slot=0;slot<TT_QWEN35_DATAFLOW_GRAPH_TRACE_SLOTS;slot++) {
				tt_qwen35_dataflow_request sample{};
				sample.version=TT_QWEN35_DATAFLOW_ABI;
				sample.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_TRACE;
				sample.next_sequence=slot;
				if (Call(fd,TT_PCI_PROBE_QWEN35_DATAFLOW_CONTROL,&sample,sizeof(sample))
					&& (sample.reserved[0]&0x80000000U)) {
					if (slot>=TT_QWEN35_DATAFLOW_GRAPH_PROFILE_FIRST) {
						static const char* names[]={"embedding","residual","rms","projection",
							"convolution","delta","silu","dense_attention","argmax","fetch",
							"authorization","sync_and_other","total"};
						static_assert(sizeof(names)/sizeof(names[0])==TT_QWEN35_DATAFLOW_GRAPH_PROFILE_BINS,
							"one diagnostic name per profile bin");
						const uint64_t cycles=uint64_t(sample.reserved[1])|(uint64_t(sample.reserved[2])<<32);
						fprintf(stderr,"P100A_Q35_DATAFLOW graph_profile bin=%s cycles=%llu\n",
							names[slot-TT_QWEN35_DATAFLOW_GRAPH_PROFILE_FIRST],
							static_cast<unsigned long long>(cycles));
					} else fprintf(stderr,"P100A_Q35_DATAFLOW graph_trace slot=%u step=%u nonzero=%u first=0x%08x last=0x%08x\n",
						slot,sample.reserved[0]&0x3ffU,(sample.reserved[0]>>10)&1U,
						sample.reserved[1],sample.reserved[2]);
				} else fprintf(stderr,"P100A_Q35_DATAFLOW graph_trace slot=%u unavailable\n",slot);
			}
		} else fputs("P100A_Q35_DATAFLOW graph_result unavailable\n",stderr);
	}
	return true;
}

// PIONEER_QWEN35_END_CLIENT_BEGIN
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
	RecoverRewrite(fd,installAttempted);
	bool serviceClosed=false,resetCompleted=false;
	const bool closeOkay=CloseRewrite(fd,authorityBound,installAttempted,
		serviceClosed,resetCompleted);
	okay=closeOkay && okay;
	tt_q38_bank_session session{};
	session.version=TT_Q38_BANK_PROTOCOL;
	bool bankEnded=!bankOpen || (okay && resetCompleted) || (okay
		&& Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))
		&& session.result.state==TT_RESIDENT_SESSION_CLOSED
		&& !session.result.active && session.result.pci_restored
		&& session.result.host_released && !session.result.cold);
	if (bankOpen && !bankEnded && !resetCompleted && !installAttempted) {
		// A failed pre-service fleet preparation can make BANK_END retain
		// a cold owner even though no dataflow service was installed.
		resetCompleted=ResetStoppedRewrite(fd,true);
		bankEnded=resetCompleted;
	}
	if (bankOpen && resetCompleted) printf("P100A_Q35_DATAFLOW unload_clean=%u reset_completed=1 service_closed=%u\n",
		unsigned(bankEnded),unsigned(serviceClosed));
	else if (bankOpen) printf("P100A_Q35_DATAFLOW unload_clean=%u state=%u active=%u "
		"cold=%u service_closed=%u\n",unsigned(bankEnded),
		session.result.state,session.result.active,session.result.cold,
		unsigned(serviceClosed));
	return bankEnded && okay;
}
// PIONEER_QWEN35_END_CLIENT_END

// PIONEER_QWEN35_EXCLUSIVE_CLIENT_BEGIN
static int OpenExclusiveDataflowDriver()
{
	const int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC|O_EXCL);
	if (fd<0) { perror("Open Qwen35 dataflow driver"); return -1; }
	// Do not trust O_EXCL on an older driver: prove that an ordinary second
	// open is rejected before any model upload, PCI configuration or launch.
	const int probe=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
	const int probeError=errno;
	if (probe>=0 || probeError!=EBUSY) {
		if (probe>=0) close(probe);
		fprintf(stderr,"P100A_Q35_DATAFLOW exclusive driver ownership could not be verified\n");
		close(fd); return -1;
	}
	return fd;
}
// PIONEER_QWEN35_EXCLUSIVE_CLIENT_END

// Authenticate and upload the actual model through the same strict preload
// used by inference, then close the bank session. No graph is installed and
// no tokenization, inference or fallback is performed in this qualification.
static bool QualifyModelPreload(const char* path)
{
	const int sourceFD=open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
	if (sourceFD<0) { perror("Open pinned GGUF"); return false; }
	std::unique_ptr<FILE,decltype(&fclose)> source(fdopen(sourceFD,"rb"),&fclose);
	if (!source) { close(sourceFD);return false; }
	PioneerQwen35Gguf packed;PioneerModelWeightCache::Digest digest{};std::string error;
	if (!packed.Open(source.get()) || !PioneerModelWeightCache::HashFile(sourceFD,packed.FullBytes,digest,error)
		|| PioneerModelWeightCache::Hex(digest)!="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
		|| !packed.Stable()) {
		fprintf(stderr,"P100A_Q35_PRELOAD_ONLY authentication_failed %s\n",error.c_str());return false;
	}
	PioneerQwen35PackedPlan plan;auto manifest=std::make_unique<D::DeviceLayout>();
	if (!plan.Build(packed,128,true) || !D::BuildDeviceLayout(packed,plan,128,*manifest)) {
		fprintf(stderr,"P100A_Q35_PRELOAD_ONLY layout_failed\n");return false;
	}
	const int fd=OpenExclusiveDataflowDriver();if (fd<0) return false;
	// PIONEER_Q35_MODEL_PRELOAD_SESSION_BEGIN
	tt_pioneer_driver_reload status{};status.version=1;
	bool okay=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&status,sizeof(status))
		&& status.driver==TT_QWEN35_DATAFLOW_DRIVER && status.build_tag==UINT64_C(0x2026112300030018)
		&& status.generation && !status.cold && !status.quiesced && !status.active_owner
		&& status.open_count==1 && status.guard_version==1;
	bool bankOpen=false,spanOpen=false;uint64_t uploadedBytes=0;
	const auto start=system_time();
	if (okay) okay=Begin(fd,bankOpen,true);
	bool uploaded=false;
	if (okay) { uploaded=Upload(fd,packed,plan,*manifest,spanOpen,uploadedBytes);okay=uploaded; }
	const bool modelValid=okay && uploadedBytes==UINT64_C(2740498432) && !spanOpen && packed.Stable();
	bool bankEnded=false;
	// A fully verified and closed upload remains safe to end even if the
	// expected byte count or source-stability check fails afterwards.
	if (uploaded && bankOpen && !spanOpen) {
		tt_q38_bank_session end{};end.version=TT_Q38_BANK_PROTOCOL;
		bankEnded=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&end,sizeof(end))
			&& end.result.state==TT_RESIDENT_SESSION_CLOSED && !end.result.active
			&& end.result.pci_restored && end.result.host_released && !end.result.cold;
	}
	okay=modelValid && bankEnded;
	const auto elapsed=system_time()-start;
	if (!okay) fprintf(stderr,"P100A_Q35_PRELOAD_ONLY failed bytes=%llu bank_open=%u span_open=%u;CLOSE_ONLY\n",
		static_cast<unsigned long long>(uploadedBytes),unsigned(bankOpen),unsigned(spanOpen));
	const bool closed=close(fd)==0;
	if (okay && closed) printf("P100A_Q35_PRELOAD_ONLY bytes=%llu load_seconds=%.6f clean_end=1;NO_INFERENCE\n",
		static_cast<unsigned long long>(uploadedBytes),double(elapsed)*1e-6);
	return okay && closed;
	// PIONEER_Q35_MODEL_PRELOAD_SESSION_END
}

// PIONEER_QWEN35_QUALIFY_CLIENT_BEGIN
// Deliberately uses the restricted qualification driver and an empty bank
// session. No GGUF, model upload, worker startup or inference is performed.
static bool QualifyRecovery()
{
	const int fd=OpenExclusiveDataflowDriver();
	if (fd<0) return false;
	tt_pioneer_driver_reload status{};status.version=1;
	bool okay=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&status,sizeof(status))
		&& status.driver==TT_QWEN35_DATAFLOW_DRIVER
		&& (status.build_tag==UINT64_C(0x2026112500030024)
			|| status.build_tag==UINT64_C(0x2026112300030001)
			|| status.build_tag==UINT64_C(0x2026112300030002)
			|| status.build_tag==UINT64_C(0x2026112400030022)) && status.generation
		&& !status.cold && !status.quiesced && !status.active_owner
		&& status.open_count==1 && status.guard_version==1;
	tt_q38_bank_session session{};session.version=TT_Q38_BANK_PROTOCOL;
	if (okay) okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))
		&& session.result.state==TT_RESIDENT_SESSION_READY && session.result.active
		&& session.result.host_ready && !session.result.cold && !session.result.uploaded_pages;
	if (okay) {
		okay=ResetStoppedRewrite(fd,true,true);
		fprintf(stderr,"P100A_Q35_RECOVERY_QUALIFY reset_completed=%u\n",unsigned(okay));
		if (!okay) {
			// Admission can reject pre-existing PCI errors before quarantine.
			// End only a demonstrably non-cold bank owner; never retry a reset.
			tt_pioneer_driver_reload current{};current.version=1;
			if (Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&current,sizeof(current))
				&& current.build_tag==status.build_tag && current.generation==status.generation
				&& !current.quiesced && current.active_owner) {
				if (current.cold) {
					tt_pioneer_pcie_evidence evidence{};evidence.version=2;
					evidence.expected_tag=status.build_tag;evidence.expected_generation=status.generation;
					bool gotEvidence=Call(fd,TT_PCI_PROBE_RECOVERY_PCIE_EVIDENCE,&evidence,sizeof(evidence));
					if (!gotEvidence) {
						// Older qualification drivers support only version 1.
						evidence={};evidence.version=1;
						evidence.expected_tag=status.build_tag;evidence.expected_generation=status.generation;
						gotEvidence=Call(fd,TT_PCI_PROBE_RECOVERY_PCIE_EVIDENCE,&evidence,sizeof(evidence));
					}
					if (gotEvidence) {
						if (evidence.version==2)
							fprintf(stderr,"P100A_Q35_PCIE_TRACE attempted_mask=0x%08x\n",evidence.reserved[0]);
						for (unsigned i=0;i<PIONEER_PCIE_TRACE_SAMPLES;i++) {
							const auto& s=evidence.samples[i];
							fprintf(stderr,"P100A_Q35_PCIE_TRACE sample=%u valid=%u dev=0x%04x link=0x%04x "
								"uncor=0x%08x mask=0x%08x severity=0x%08x root=0x%08x source=0x%08x "
								"header=%08x,%08x,%08x,%08x\n",i,s.valid,s.device_status,s.link_status,
								s.uncorrectable,s.uncorrectable_mask,s.severity,s.root_status,s.source,
								s.header[0],s.header[1],s.header[2],s.header[3]);
						}
					} else fprintf(stderr,"P100A_Q35_PCIE_TRACE unavailable\n");
				} else {
					tt_q38_bank_session end{};end.version=TT_Q38_BANK_PROTOCOL;
					(void)Call(fd,TT_PCI_PROBE_Q38_BANK_END,&end,sizeof(end));
				}
			}
		}
	} else {
		// A failed or lost begin reply may still have created an owner. Inspect
		// CPU metadata before choosing normal end or guarded fault recovery.
		tt_pioneer_driver_reload current{};current.version=1;
		if (Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&current,sizeof(current))
			&& (current.build_tag==UINT64_C(0x2026112500030024)
				|| current.build_tag==UINT64_C(0x2026112300030001)
				|| current.build_tag==UINT64_C(0x2026112300030002)
				|| current.build_tag==UINT64_C(0x2026112400030022)) && current.active_owner) {
			if (current.cold) (void)ResetStoppedRewrite(fd,true);
			else {
				tt_q38_bank_session end{};end.version=TT_Q38_BANK_PROTOCOL;
				(void)Call(fd,TT_PCI_PROBE_Q38_BANK_END,&end,sizeof(end));
			}
		}
	}
	const bool closed=close(fd)==0;
	return okay && closed;
}
// PIONEER_QWEN35_QUALIFY_CLIENT_END

// PIONEER_QWEN35_FLEET_INSPECTION_CLIENT_BEGIN
static bool QualifyHeldFleet(bool stageRoles=false,bool prestack=false,bool localRestore=false,bool service=false,
	bool fault=false,bool fleetService=false,bool resetSample=false)
{
	if ((resetSample && (stageRoles || prestack || localRestore || service || fault || fleetService))
		|| (prestack && !stageRoles) || (localRestore && (!stageRoles || !prestack))
		|| (service && !localRestore) || (fault && !service)
		|| (fleetService && (!service || fault))) return false;
	const int fd=OpenExclusiveDataflowDriver();
	if (fd<0) return false;
	tt_pioneer_driver_reload initial{};initial.version=1;
	bool okay=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&initial,sizeof(initial))
		&& initial.driver==TT_QWEN35_DATAFLOW_DRIVER
		&& initial.build_tag==(resetSample?UINT64_C(0x2026112400030023):fleetService?UINT64_C(0x2026112300030021):fault?UINT64_C(0x2026112300030013):service?UINT64_C(0x2026112300030012):localRestore?UINT64_C(0x2026112300030011):prestack?UINT64_C(0x2026112300030010)
			:stageRoles?UINT64_C(0x2026112300030005):UINT64_C(0x2026112300030004)) && initial.generation
		&& !initial.cold && !initial.quiesced && !initial.active_owner
		&& initial.open_count==1 && initial.guard_version==1;
	if (okay) {
		tt_q38_bank_session session{};session.version=TT_Q38_BANK_PROTOCOL;
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))
			&& session.result.state==TT_RESIDENT_SESSION_READY && session.result.active
			&& session.result.host_ready && !session.result.cold && !session.result.uploaded_pages;
		if (okay) {
			tt_q35_fleet_inspection result{};result.version=1;
			result.expected_tag=initial.build_tag;result.expected_generation=initial.generation;
			const bool replied=Call(fd,TT_PCI_PROBE_Q35_FLEET_INSPECT,&result,sizeof(result));
			fprintf(stderr,"P100A_Q35_FLEET_INSPECTION replied=%u status=%ld stage=%u core=%u "
				"l2cpu=0x%08x restore_failed=%u prepared=%u\n",unsigned(replied),
				long(result.operation_status),result.stage,result.failed_core,result.l2cpu_reset,
				result.restore_failed,result.prepared);
			okay=replied && result.operation_status==B_OK
				&& result.build_tag==initial.build_tag && result.generation==initial.generation
				&& result.stage==TT_Q35_FLEET_INSPECTION_COMPLETE && result.failed_core==120
				&& result.l2cpu_reset!=UINT32_MAX && !(result.l2cpu_reset&0xf0U)
				&& !result.restore_failed && result.prepared==120;
			if (okay && resetSample) {
				tt_q35_risc_reset_sample sample{};sample.version=1;
				sample.expected_tag=initial.build_tag;sample.expected_generation=initial.generation;
				const bool sampleReply=Call(fd,TT_PCI_PROBE_Q35_RISC_RESET_SAMPLE,&sample,sizeof(sample));
				fprintf(stderr,"P100A_Q35_RISC_RESET_SAMPLE replied=%u status=%ld sampled=%u restore_failed=%u "
					"words=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
					unsigned(sampleReply),long(sample.operation_status),sample.sampled,sample.restore_failed,
					sample.words[0],sample.words[1],sample.words[2],sample.words[3],
					sample.words[4],sample.words[5],sample.words[6],sample.words[7]);
				okay=sampleReply && sample.operation_status==B_OK && sample.sampled==1
					&& !sample.restore_failed && sample.build_tag==initial.build_tag
					&& sample.generation==initial.generation;
			}
			if (okay && stageRoles) {
				tt_q35_role_stage_qualification role{};role.version=fleetService?6:fault?5:service?4:localRestore?3:prestack?2:1;
				role.expected_tag=initial.build_tag;role.expected_generation=initial.generation;
				const bool stageReply=Call(fd,TT_PCI_PROBE_Q35_ROLE_STAGE_QUALIFY,&role,sizeof(role));
				fprintf(stderr,"P100A_Q35_ROLE_STAGE replied=%u status=%ld stage=%u captured=%u "
					"staged=%u restored=%u mappings_closed=%u\n",unsigned(stageReply),
					long(role.operation_status),role.stage,role.captured,role.staged,
					role.restored,role.mappings_closed);
				okay=stageReply && role.operation_status==B_OK
					&& role.build_tag==initial.build_tag && role.generation==initial.generation
					&& role.stage==(fleetService?TT_Q35_ROLE_FLEET_SERVICE_COMPLETE:fault?TT_Q35_ROLE_SERVICE_FAULT_COMPLETE:service?TT_Q35_ROLE_SERVICE_COMPLETE:localRestore?TT_Q35_ROLE_LOCAL_RESTORE_COMPLETE:
						prestack?TT_Q35_ROLE_PRESTACK_COMPLETE:TT_Q35_ROLE_STAGE_COMPLETE) && role.captured==1
					&& role.staged==1 && role.restored==1 && role.mappings_closed==1;
			}
		}
		// Even a lost BEGIN or inspection reply may leave a session. Only end
		// a same-generation non-cold owner; unknown state is retained on close.
		tt_pioneer_driver_reload current{};current.version=1;
		const bool known=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&current,sizeof(current))
			&& current.build_tag==initial.build_tag && current.generation==initial.generation
			&& current.driver==TT_QWEN35_DATAFLOW_DRIVER && current.guard_version==1
			&& !current.quiesced && current.open_count==1;
		bool ended=false;
		if (known && current.active_owner && !current.cold) {
			tt_q38_bank_session end{};end.version=TT_Q38_BANK_PROTOCOL;
			ended=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&end,sizeof(end))
				&& end.result.state==TT_RESIDENT_SESSION_CLOSED && !end.result.active && !end.result.cold;
		}
		fprintf(stderr,"P100A_Q35_FLEET_INSPECTION clean_end=%u\n",unsigned(ended));
		okay=okay && ended;
	}
	const bool closed=close(fd)==0;
	return okay && closed;
}
// PIONEER_QWEN35_FLEET_INSPECTION_CLIENT_END

// PIONEER_QWEN35_PROJECTION_CLIENT_BEGIN
// Run only the driver's fixed diagnostic, with the caller's exact candidate
// identity. A lost or incomplete projection reply is never treated as clean.
static bool QualifyFixedProjection(uint64_t expectedTag,uint64_t expectedGeneration)
{
	if (!expectedTag || !expectedGeneration) return false;
	const int fd=OpenExclusiveDataflowDriver();
	if (fd<0) return false;
	tt_pioneer_driver_reload initial{};initial.version=1;
	bool okay=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&initial,sizeof(initial))
		&& initial.driver==TT_QWEN35_DATAFLOW_DRIVER && initial.build_tag==expectedTag
		&& initial.generation==expectedGeneration && !initial.cold && !initial.quiesced
		&& !initial.active_owner && initial.open_count==1 && initial.guard_version==1;
	bool attemptedBegin=false,attemptedProjection=false,replied=false,cleanReply=false;
	tt_q35_projection_qualification result{};result.version=1;
	result.expected_tag=expectedTag;result.expected_generation=expectedGeneration;
	if (okay) {
		tt_q38_bank_session begin{};begin.version=TT_Q38_BANK_PROTOCOL;
		attemptedBegin=true;
		okay=Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&begin,sizeof(begin))
			&& begin.result.state==TT_RESIDENT_SESSION_READY && begin.result.active
			&& begin.result.host_ready && !begin.result.cold && !begin.result.uploaded_pages;
		if (okay) {
			attemptedProjection=true;
			replied=Call(fd,TT_PCI_PROBE_Q35_PROJECTION_QUALIFY,&result,sizeof(result));
			const bool identity=replied && result.version==1 && !result.reserved
				&& result.expected_tag==expectedTag && result.expected_generation==expectedGeneration
				&& result.build_tag==expectedTag && result.generation==expectedGeneration;
			// An early error can also be clean: the transaction restores and
			// abandons its unrun scratch before reporting stages 2 through 6.
			const bool cleanPrelaunch=result.stage>=2 && result.stage<=6
				&& result.operation_status!=B_OK && !result.output_verified;
			const bool cleanFinished=result.stage==15 && result.output_verified<=1;
			cleanReply=identity && result.cleanup_complete==1 && (cleanPrelaunch || cleanFinished);
			okay=cleanReply && result.operation_status==B_OK && result.stage==15
				&& result.output_verified==1;
			fprintf(stderr,"P100A_Q35_FIXED_PROJECTION replied=%u status=%ld stage=%u "
				"route_stage=%u route_core=%u output_verified=%u cleanup_complete=%u\n",
				unsigned(replied),long(result.operation_status),result.stage,result.route_stage,
				result.route_core,result.output_verified,result.cleanup_complete);
		}
	}
	bool ended=false;
	if (attemptedBegin && (!attemptedProjection || cleanReply)) {
		tt_pioneer_driver_reload current{};current.version=1;
		const bool known=Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&current,sizeof(current))
			&& current.driver==TT_QWEN35_DATAFLOW_DRIVER && current.build_tag==expectedTag
			&& current.generation==expectedGeneration && !current.cold && !current.quiesced
			&& current.active_owner && current.open_count==1 && current.guard_version==1;
		if (known) {
			tt_q38_bank_session end{};end.version=TT_Q38_BANK_PROTOCOL;
			ended=Call(fd,TT_PCI_PROBE_Q38_BANK_END,&end,sizeof(end))
				&& end.result.state==TT_RESIDENT_SESSION_CLOSED && !end.result.active
				&& end.result.pci_restored && end.result.host_released && !end.result.cold;
		}
	}
	if (attemptedBegin) fprintf(stderr,"P100A_Q35_FIXED_PROJECTION bank_end_clean=%u\n",unsigned(ended));
	const bool closed=close(fd)==0;
	return okay && ended && closed;
}
static bool ParseProjectionIdentity(const char* tagText,const char* generationText,
	uint64_t& tag,uint64_t& generation)
{
	// Build tags are printed as exactly sixteen hexadecimal digits; reject
	// signs, prefixes, overflow, whitespace and zero before opening the driver.
	if (!tagText || strlen(tagText)!=16 || !generationText || !*generationText) return false;
	for (const char* p=tagText;*p;p++)
		if (!((*p>='0' && *p<='9') || (*p>='a' && *p<='f') || (*p>='A' && *p<='F'))) return false;
	for (const char* p=generationText;*p;p++) if (*p<'0' || *p>'9') return false;
	char* end=nullptr;errno=0;
	const unsigned long long parsedTag=strtoull(tagText,&end,16);
	if (errno || *end || !parsedTag) return false;
	errno=0;const unsigned long long parsedGeneration=strtoull(generationText,&end,10);
	if (errno || *end || !parsedGeneration) return false;
	tag=parsedTag;generation=parsedGeneration;return true;
}
// PIONEER_QWEN35_PROJECTION_CLIENT_END

int main(int argc,char** argv)
{
	setvbuf(stdout,nullptr,_IOLBF,0);
	if (argc==4 && strcmp(argv[1],"--qualify-fixed-projection")==0) {
		uint64_t tag=0,generation=0;
		if (!ParseProjectionIdentity(argv[2],argv[3],tag,generation)) return 2;
		return QualifyFixedProjection(tag,generation)?0:1;
	}
	if (argc==3 && strcmp(argv[1],"--qualify-model-preload")==0)
		return QualifyModelPreload(argv[2])?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-risc-reset-sample")==0)
		return QualifyHeldFleet(false,false,false,false,false,false,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--inspect-held-fleet")==0)
		return QualifyHeldFleet()?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-held-role-stage")==0)
		return QualifyHeldFleet(true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-role-prestack")==0)
		return QualifyHeldFleet(true,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-role-local-restore")==0)
		return QualifyHeldFleet(true,true,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-role-service")==0)
		return QualifyHeldFleet(true,true,true,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-role-service-fault")==0)
		return QualifyHeldFleet(true,true,true,true,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-role-fleet-service")==0)
		return QualifyHeldFleet(true,true,true,true,false,true)?0:1;
	if (argc==2 && strcmp(argv[1],"--qualify-recovery")==0)
		return QualifyRecovery()?0:1;
	const bool oneRunDiagnostic=argc==7
		&& strcmp(argv[6],"--diagnostic-one-run")==0;
	if (argc<5 || argc>7 || (argc==7 && !oneRunDiagnostic)) {
		fprintf(stderr,"usage: %s GGUF ABSOLUTE_VOCAB_LIBRARY PROMPT "
			"MAX_NEW_TOKENS [CONTEXT] [--diagnostic-one-run]\n",argv[0]);
		return 2;
	}
	const unsigned context=argc>=6?Parse(argv[5],D::MaxPositions):128;
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

	const int fd=OpenExclusiveDataflowDriver();
	if (fd<0) return 1;
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
		firstTokens,firstText,oneRunDiagnostic);
	const bool secondOkay=oneRunDiagnostic || (firstOkay
		&& Generate(fd,vocabulary,prompt,maxNewTokens,2,secondTokens,secondText,false));
	const bool firstPrefix=firstTokens.size()>=2
		&& firstTokens[0]==11751 && firstTokens[1]==13;
	const bool secondPrefix=oneRunDiagnostic || (secondTokens.size()>=2
		&& secondTokens[0]==11751 && secondTokens[1]==13);
	const bool exactPrefix=firstPrefix && secondPrefix;
	const bool repeatable=oneRunDiagnostic
		|| (firstTokens==secondTokens && firstText==secondText);
	if (oneRunDiagnostic)
		printf("P100A_Q35_DATAFLOW diagnostic_one_run=1\n");
	if (!exactPrefix) fprintf(stderr,
		"P100A_Q35_DATAFLOW reference prefix mismatch expected=11751,13\n");
	if (!repeatable) fprintf(stderr,
		"P100A_Q35_DATAFLOW warm outputs differ\n");
	const bool ended=End(fd,bankOpen,spanOpen,authorityBound,installAttempted);
	const bool closed=close(fd)==0;
	return firstOkay && secondOkay && exactPrefix && repeatable && ended && closed?0:1;
}
