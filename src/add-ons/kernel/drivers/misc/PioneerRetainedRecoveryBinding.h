/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RETAINED_RECOVERY_BINDING_H
#define PIONEER_RETAINED_RECOVERY_BINDING_H
#include "PioneerRetainedRecoveryModuleAdapters.h"
#include "PioneerRetainedRecoveryScope.h"

// An owned, already-acquired pair of node pins and one retained controller
// cookie. The caller retains both module references and the owner identity,
// serializes calls, and establishes actual PCI, DMA, MMIO and firmware
// authority separately. This object never automatically releases resources.
// Retirement is permitted only after the recovery operation supplies an
// independently verified post-reset proof; a false assertion here is unsafe.
template<class PinService,class ControllerService> class PioneerRetainedRecoveryBinding {
	using Pin=PioneerRetainedRecoveryPinAdapter<PinService>;
	using Controller=PioneerRetainedRecoveryControllerAdapter<ControllerService>;
	using Scope=PioneerRetainedRecoveryScope<Pin,Controller>;
	PinService* fPins;
	ControllerService* fControllerService;
	void* fCookie;
	const void* fOwner;
	uint64_t fSession,fCardId,fRootId;
	Pin fCard,fRoot;
	Controller fController;
	Scope fScope;
	bool fAwaitingProof=false,fRetirementStarted=false,fRetired=false,fFaulted=false;
	struct Retirement {
		PioneerRetainedRecoveryBinding& binding;
		static status_t ControllerCall(void* opaque) {
			auto& b=static_cast<Retirement*>(opaque)->binding;
			const uint64_t ticket=b.fScope.Ticket();
			status_t status=b.fControllerService->resume(b.fCookie,b.fOwner,b.fSession,ticket);
			if (status!=B_OK) return status;
			status=b.fControllerService->finish(b.fCookie,b.fOwner,b.fSession,ticket,true);
			if (status!=B_OK)
				(void)b.fControllerService->park(b.fCookie,b.fOwner,b.fSession,ticket);
			return status;
		}
		static status_t CardCall(void* opaque) {
			auto& call=*static_cast<Retirement*>(opaque);auto& b=call.binding;status_t result=B_ERROR;
			const status_t status=b.fPins->run(b.fRootId,b.fOwner,b.fSession,ControllerCall,&call,&result);
			return status==B_OK?result:status;
		}
	};
public:
	using Result=typename Scope::Result;
	using Operation=typename Scope::Operation;
	PioneerRetainedRecoveryBinding(PinService* pins,ControllerService* controller,void* cookie,
		const void* owner,uint64_t session,uint64_t cardId,uint64_t rootId)
		:fPins(pins),fControllerService(controller),fCookie(cookie),fOwner(owner),fSession(session),
		 fCardId(cardId),fRootId(rootId),fCard(pins,cardId),fRoot(pins,rootId),
		 fController(controller,cookie),fScope(fCard,fRoot,fController,owner,session,cardId,rootId) {}
	PioneerRetainedRecoveryBinding(const PioneerRetainedRecoveryBinding&)=delete;
	PioneerRetainedRecoveryBinding& operator=(const PioneerRetainedRecoveryBinding&)=delete;
	Result Step(Operation operation,void* argument,status_t& operationStatus) {
		operationStatus=B_ERROR;
		if (!fPins || !fPins->run || !fPins->release || !fControllerService
			|| !fControllerService->begin || !fControllerService->park
			|| !fControllerService->resume || !fControllerService->finish
			|| !fCookie || !fOwner || !fSession || !fCardId || !fRootId || fCardId==fRootId
			|| fAwaitingProof || fRetirementStarted || fFaulted || fRetired)
			return Scope::Rejected;
		const Result result=fScope.Step(operation,argument,operationStatus);
		if (result==Scope::AwaitingProof) fAwaitingProof=true;
		else if (result!=Scope::Pending) fFaulted=true;
		return result;
	}
	status_t Retire(bool verifiedPostResetProof) {
		if (!verifiedPostResetProof || !fAwaitingProof || fRetirementStarted || fFaulted || fRetired)
			return B_NOT_ALLOWED;
		fRetirementStarted=true;
		Retirement call{*this};status_t result=B_ERROR;
		status_t status=fPins->run(fCardId,fOwner,fSession,Retirement::CardCall,&call,&result);
		if (status!=B_OK || result!=B_OK) { fFaulted=true;return status!=B_OK?status:result; }
		status=fPins->release(fRootId,fOwner,fSession,true);
		if (status!=B_OK) { fFaulted=true;return status; }
		fRootId=0;
		status=fPins->release(fCardId,fOwner,fSession,true);
		if (status!=B_OK) { fFaulted=true;return status; }
		fCardId=0;fRetired=true;return B_OK;
	}
	bool Retired() const { return fRetired; }
	uint64_t Ticket() const { return fScope.Ticket(); }
};
#endif
