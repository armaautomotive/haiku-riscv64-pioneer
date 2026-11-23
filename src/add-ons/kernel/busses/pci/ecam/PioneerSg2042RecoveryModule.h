/* Distributed under the MIT License. */
#ifndef PIONEER_SG2042_RECOVERY_MODULE_H
#define PIONEER_SG2042_RECOVERY_MODULE_H
#include <module.h>

#define PIONEER_SG2042_RECOVERY_MODULE "busses/pci/ecam/pioneer_recovery/v1"

// Compile witness only: its exported callbacks refuse all requests, and the
// ordinary ECAM driver does not export this module. Before any loadable
// version, resolve legacy read/write error propagation and owner teardown.
// Kernel-only. cookie must be the retained ECAM driver cookie for the exact
// private Pioneer FDT root. Begin/End must run on the same locked driver
// thread; the caller must end the lease before dropping its node/module refs.
// This controls PCI config calls through this controller only, not DMA/MMIO.
struct pioneer_sg2042_recovery_module_info {
	module_info info;
	status_t (*begin)(void* cookie,uint64* ticket);
	status_t (*end)(void* cookie,uint64 ticket);
};
#endif
