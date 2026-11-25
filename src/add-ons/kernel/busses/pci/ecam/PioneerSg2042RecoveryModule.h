/* Distributed under the MIT License. */
#ifndef PIONEER_SG2042_RECOVERY_MODULE_H
#define PIONEER_SG2042_RECOVERY_MODULE_H
#include <module.h>

#define PIONEER_SG2042_RECOVERY_MODULE "busses/pci/ecam/pioneer_recovery/v1"

// Compile witness only: the normal witness refuses every request. The optional
// gateway witness forwards to the real controller lease, but still refuses
// controller initialization and cannot be loaded. The ordinary ECAM driver
// does not export this module. The callback is executed
// synchronously on the calling thread; the controller releases its lease before
// returning. The retained controller cookie and module must outlive the call.
// This controls controller-mediated PCI config access only, not DMA or MMIO.
// The operation must not wait for a thread that may require this controller.
typedef status_t (*pioneer_sg2042_recovery_operation)(void* argument);
struct pioneer_sg2042_recovery_module_info {
	module_info info;
	status_t (*run)(void* cookie, pioneer_sg2042_recovery_operation operation,
		void* argument, status_t* operationStatus);
};
#endif
