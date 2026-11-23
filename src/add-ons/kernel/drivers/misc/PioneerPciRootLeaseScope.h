/* Distributed under the MIT License. */
#ifndef PIONEER_PCI_ROOT_LEASE_SCOPE_H
#define PIONEER_PCI_ROOT_LEASE_SCOPE_H
#include <stdint.h>

// A single locked recovery call borrows the ECAM module and retained root
// cookie. The scope never stores either beyond its own lifetime. A failed or
// ambiguous release is not repeated and must quarantine the caller's owner.
// IO::Quarantine() is kernel-memory-only; it must not touch PCI or MMIO.
template<class IO,class Service> class PioneerPciRootLeaseScope {
	IO& io;
	Service* service=nullptr;
	void* cookie=nullptr;
	uint64_t ticket=0;
	bool attempted=false,active=false,closeAttempted=false;
public:
	explicit PioneerPciRootLeaseScope(IO& owned):io(owned) {}
	PioneerPciRootLeaseScope(const PioneerPciRootLeaseScope&)=delete;
	PioneerPciRootLeaseScope& operator=(const PioneerPciRootLeaseScope&)=delete;
	~PioneerPciRootLeaseScope() { if (active && !closeAttempted) (void)Close(); }
	bool Begin(Service* module,void* controller) {
		if (attempted || !module || !module->begin || !module->end || !controller) return false;
		attempted=true;service=module;cookie=controller;
		const status_t status=service->begin(cookie,&ticket);
		if (status!=B_OK || !ticket) {
			if (ticket || status==B_OK) io.Quarantine();
			return false;
		}
		active=true;return true;
	}
	status_t Close() {
		if (!active || closeAttempted) return B_NOT_ALLOWED;
		closeAttempted=true;
		const status_t status=service->end(cookie,ticket);
		if (status!=B_OK) { io.Quarantine();return status; }
		active=false;return B_OK;
	}
	bool Active() const { return active; }
};
#endif
