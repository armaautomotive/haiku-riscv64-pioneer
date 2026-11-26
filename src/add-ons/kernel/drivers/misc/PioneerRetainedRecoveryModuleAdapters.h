/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_RETAINED_RECOVERY_MODULE_ADAPTERS_H
#define PIONEER_RETAINED_RECOVERY_MODULE_ADAPTERS_H
#include <stdint.h>

// These adapters borrow already-retained module references and identities.
// Their owner must keep the module, cookie, node pins and session alive until
// independent release proofs exist; a destructor deliberately releases none.
template<class Service> class PioneerRetainedRecoveryPinAdapter {
	Service* fService;
	uint64_t fId;
public:
	PioneerRetainedRecoveryPinAdapter(Service* service,uint64_t id):fService(service),fId(id) {}
	status_t Run(const void* owner,uint64_t session,uint64_t id,
		status_t (*operation)(void*),void* argument,status_t* result) {
		if (!fService || !fService->run || !fId || id!=fId) return B_BAD_VALUE;
		return fService->run(fId,owner,session,operation,argument,result);
	}
};

template<class Service> class PioneerRetainedRecoveryControllerAdapter {
	Service* fService;
	void* fCookie;
public:
	PioneerRetainedRecoveryControllerAdapter(Service* service,void* cookie)
		:fService(service),fCookie(cookie) {}
	status_t BeginPioneerRetainedRecovery(const void* owner,uint64_t session,uint64_t& ticket) {
		ticket=0;
		if (!fService || !fService->begin || !fCookie) return B_BAD_VALUE;
		return fService->begin(fCookie,owner,session,&ticket);
	}
	status_t ParkPioneerRetainedRecovery(const void* owner,uint64_t session,uint64_t ticket) {
		if (!fService || !fService->park || !fCookie) return B_BAD_VALUE;
		return fService->park(fCookie,owner,session,ticket);
	}
	status_t ResumePioneerRetainedRecovery(const void* owner,uint64_t session,uint64_t ticket) {
		if (!fService || !fService->resume || !fCookie) return B_BAD_VALUE;
		return fService->resume(fCookie,owner,session,ticket);
	}
};
#endif
