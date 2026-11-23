/* Distributed under the MIT License. */
#ifndef PIONEER_PCI_ROOT_LEASE_SCOPE_H
#define PIONEER_PCI_ROOT_LEASE_SCOPE_H

// The service owns the lease for the duration of one synchronous call. The
// caller borrows the module and retained controller cookie only for that call.
// Quarantine is memory-only. Any failure after entry into the operation has
// uncertain release state; never retry it automatically.
template<class IO,class Service> class PioneerPciRootLeaseScope {
	IO& io;
	bool attempted=false;
	struct Invocation {
		status_t (*operation)(void*);
		void* argument;
		unsigned calls=0;
	};
	static status_t Invoke(void* opaque) {
		Invocation& invocation=*static_cast<Invocation*>(opaque);
		if (++invocation.calls!=1) return B_NOT_ALLOWED;
		return invocation.operation(invocation.argument);
	}
public:
	explicit PioneerPciRootLeaseScope(IO& owned):io(owned) {}
	PioneerPciRootLeaseScope(const PioneerPciRootLeaseScope&)=delete;
	PioneerPciRootLeaseScope& operator=(const PioneerPciRootLeaseScope&)=delete;
	status_t Run(Service* service,void* cookie,status_t (*operation)(void*),
		void* argument,status_t* operationStatus) {
		if (attempted || !service || !service->run || !cookie || !operation || !operationStatus)
			return B_NOT_ALLOWED;
		attempted=true;
		Invocation invocation{operation,argument};
		const status_t status=service->run(cookie,Invoke,&invocation,operationStatus);
		if (invocation.calls>1 || (status==B_OK && invocation.calls!=1)
			|| (status!=B_OK && invocation.calls!=0)) {
			io.Quarantine();
			return status==B_OK ? B_ERROR : status;
		}
		return status;
	}
};
#endif
