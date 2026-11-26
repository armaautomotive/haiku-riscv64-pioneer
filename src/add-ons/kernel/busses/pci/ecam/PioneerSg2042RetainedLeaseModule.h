/* Distributed under the MIT License. */
#ifndef PIONEER_SG2042_RETAINED_LEASE_MODULE_H
#define PIONEER_SG2042_RETAINED_LEASE_MODULE_H
#include <module.h>

#define PIONEER_SG2042_RETAINED_LEASE_MODULE "busses/pci/ecam/pioneer_retained_lease/v1"

// The caller must retain the matching initialized controller node and this
// module, serialize the recovery, and hold the owner/session until release.
// A parked lease excludes controller-mediated configuration only; it does
// not establish topology, MMIO, DMA or firmware exclusion, or reset safety.
// This experimental module is only available in a controller whose init
// rejects use, until the complete recovery path is physically qualified.
struct pioneer_sg2042_retained_lease_module_info {
	module_info info;
	status_t (*begin)(void* cookie, const void* owner, uint64 session, uint64* ticket);
	status_t (*park)(void* cookie, const void* owner, uint64 session, uint64 ticket);
	status_t (*resume)(void* cookie, const void* owner, uint64 session, uint64 ticket);
	status_t (*finish)(void* cookie, const void* owner, uint64 session, uint64 ticket,
		bool releaseAuthorized);
};
#endif
