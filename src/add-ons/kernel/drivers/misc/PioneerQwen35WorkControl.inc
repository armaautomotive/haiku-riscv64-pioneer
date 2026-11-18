/* SPDX-License-Identifier: MIT */
#ifdef TT_PIONEER_Q35_WORK
#include "../../../../../tools/pioneer/PioneerQwen35ManifestValidation.h"

static void
Q35RetainWork(PioneerStoreContext& context)
{
	tt_resident_session_result retained={};
	RetainQ38DispatchOwner(context,retained);
}

static status_t
Q35WorkControl(PioneerProbeCookie* cookie,uint32 operation,void* data,size_t length)
{
	using namespace PioneerQwen35;
	if (!data) return B_BAD_VALUE;
	thread_info thread={};
	status_t status=get_thread_info(find_thread(NULL),&thread);
	if (status!=B_OK) return status;
	if (thread.team!=cookie->team) return B_NOT_ALLOWED;
	status=AcquireProbeLock(cookie,true);
	if (status!=B_OK) return status;
	auto* context=cookie->context;
	if (operation==TT_PCI_PROBE_Q35_WORK_SUPPORT && length==sizeof(WorkSupport)) {
		const WorkSupport support={WorkVersion,WorkDriver,MaxPositions,Vocabulary,1,0,1,0};
		status=user_memcpy(data,&support,sizeof(support));
	} else if (!context || sRuntimeOwner!=cookie || !context->bankStorageOnly || context->Unsafe()) {
		status=B_NOT_ALLOWED;
	} else if (operation==TT_PCI_PROBE_Q35_WORK_PREPARE && length==sizeof(WorkPrepare)) {
		if (context->q35Work || !context->q38Span || !context->q38Span->closed)
			status=B_NOT_ALLOWED;
		else {
			auto* request=new(std::nothrow) WorkPrepare;
			if (!request) status=B_NO_MEMORY;
			else {
				status=user_memcpy(request,data,sizeof(*request));
				if (status==B_OK) {
					constexpr unsigned pagesPerBank=PioneerResidentBanks::Capacity/4096;
					auto initialized=[&](unsigned bank,unsigned page) {
						return context->initialized[bank*pagesPerBank+page]!=0;
					};
					if (request->version!=WorkVersion || request->reserved
						|| !ValidateManifest(request->manifest,request->rope,initialized,
							context->uploadedPages)) status=B_BAD_VALUE;
					else {
						auto& dram=context->baseline.baseline.baseline.baseline.dram;
						auto& noc=context->baseline.baseline.baseline.noc;
						auto* binding=new(std::nothrow) PioneerQ35WorkBinding(context->io,
							dram,noc,context->lease,context->banks);
						if (!binding) status=B_NO_MEMORY;
						else {
							context->q35Work=binding;
							if (!binding->Map() || !binding->owner.Prepare(*binding,*request)) {
								Q35RetainWork(*context);
								status=B_IO_ERROR;
							}
						}
					}
				}
				delete request;
			}
		}
	} else if (!context->q35Work) {
		status=B_NOT_ALLOWED;
	} else if (operation==TT_PCI_PROBE_Q35_WORK_START && length==sizeof(WorkStart)) {
		auto* request=new(std::nothrow) WorkStart;
		if (!request) status=B_NO_MEMORY;
		else {
			status=user_memcpy(request,data,sizeof(*request));
			auto& binding=*context->q35Work;
			if (status==B_OK) {
				if (!binding.owner.CanStart(*request)) status=B_BAD_VALUE;
				else if (!binding.owner.Start(binding,*request)) {
					Q35RetainWork(*context);
					status=B_IO_ERROR;
				}
			}
			delete request;
		}
	} else if (operation==TT_PCI_PROBE_Q35_WORK_POLL && length==sizeof(WorkPoll)) {
		auto* result=new(std::nothrow) WorkPoll{};
		if (!result) status=B_NO_MEMORY;
		else {
			unsigned input[2];
			status=user_memcpy(input,data,sizeof(input));
			auto& binding=*context->q35Work;
			if (status==B_OK) {
				if (input[0]!=WorkVersion || !binding.owner.Active()
					|| input[1]!=binding.owner.RequestId()) status=B_BAD_VALUE;
				else {
					result->version=input[0];
					result->requestId=input[1];
					if (!binding.owner.Poll(binding,*result)) {
						Q35RetainWork(*context);
						result->state=WorkQuarantined;
					}
					status=user_memcpy(data,result,sizeof(*result));
					if (status!=B_OK) Q35RetainWork(*context);
				}
			}
			delete result;
		}
	} else if (operation==TT_PCI_PROBE_Q35_WORK_STOP && length==sizeof(WorkStop)) {
		WorkStop stop={};
		status=user_memcpy(&stop,data,sizeof(stop));
		auto& binding=*context->q35Work;
		if (status==B_OK) {
			if (stop.version!=WorkVersion || stop.restored || stop.reserved[0] || stop.reserved[1])
				status=B_BAD_VALUE;
			else if (binding.owner.Active()) status=B_BUSY;
			else {
				stop.restored=binding.owner.Restored() || binding.owner.Stop(binding);
				if (!stop.restored) Q35RetainWork(*context);
				status=user_memcpy(data,&stop,sizeof(stop));
			}
		}
	} else status=B_BAD_VALUE;
	release_sem(sLock);
	return status;
}
#endif
