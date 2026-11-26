/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ECAM_LEASE_PROBE_LOGIC_H
#define PIONEER_ECAM_LEASE_PROBE_LOGIC_H
// Called only with the profile-validated controller cookie and both device
// node pins held. The qualification module supplies its own fixed callback.
struct PioneerEcamLeaseProbeCall {
	pioneer_ecam_lease_probe_result* result;
	static status_t UnexpectedCallback(void* opaque) {
		++*static_cast<uint32*>(opaque);
		return B_OK;
	}
	static status_t Run(void* cookie, void* opaque) {
		PioneerEcamLeaseProbeCall& call = *static_cast<PioneerEcamLeaseProbeCall*>(opaque);
		if (cookie == NULL) return B_BAD_VALUE;
		pioneer_sg2042_lease_probe_module_info* probe = NULL;
		call.result->leaseModuleStatus = get_module(PIONEER_SG2042_LEASE_PROBE_MODULE,
			(module_info**)&probe);
		if (call.result->leaseModuleStatus != B_OK) return call.result->leaseModuleStatus;
		if (probe != NULL && probe->probe != NULL)
			call.result->leaseStatus = probe->probe(cookie, &call.result->leaseCallbacks,
				&call.result->leaseCallbackStatus);
		put_module(PIONEER_SG2042_LEASE_PROBE_MODULE);
		if (call.result->leaseStatus != B_OK || call.result->leaseCallbacks != 1
			|| call.result->leaseCallbackStatus != B_CANCELED) return B_ERROR;
		pioneer_sg2042_recovery_module_info* recovery = NULL;
		call.result->recoveryModuleStatus = get_module(PIONEER_SG2042_RECOVERY_MODULE,
			(module_info**)&recovery);
		if (call.result->recoveryModuleStatus != B_OK) return call.result->recoveryModuleStatus;
		status_t ignored = B_ERROR;
		if (recovery != NULL && recovery->run != NULL)
			call.result->recoveryStatus = recovery->run(cookie, UnexpectedCallback,
				&call.result->recoveryCallbacks, &ignored);
		put_module(PIONEER_SG2042_RECOVERY_MODULE);
		return call.result->recoveryStatus == B_NOT_ALLOWED
			&& call.result->recoveryCallbacks == 0 ? B_OK : B_ERROR;
	}
};

#ifdef PIONEER_ECAM_CONTENTION_QUALIFICATION
struct PioneerEcamLeaseHoldCall {
	pioneer_ecam_lease_hold* result;
	static status_t Run(void* cookie, void* opaque) {
		PioneerEcamLeaseHoldCall& call = *static_cast<PioneerEcamLeaseHoldCall*>(opaque);
		if (cookie == NULL) return B_BAD_VALUE;
		pioneer_sg2042_lease_contention_module_info* probe = NULL;
		status_t status = get_module(PIONEER_SG2042_LEASE_CONTENTION_MODULE, (module_info**)&probe);
		if (status != B_OK) return status;
		if (probe != NULL && probe->hold != NULL)
			call.result->holdStatus = probe->hold(cookie, call.result->ready, call.result->release,
				&call.result->callbackStatus);
		put_module(PIONEER_SG2042_LEASE_CONTENTION_MODULE);
		return call.result->holdStatus == B_OK && call.result->callbackStatus == B_OK ? B_OK : B_ERROR;
	}
};

// This path uses only pinned device-tree metadata; it does not issue config
// reads, so it can inspect a controller while another thread waits on its lease.
struct PioneerEcamLeaseStateCall {
	device_manager_info* dm;
	pioneer_ecam_lease_state* result;
	static status_t Run(device_node* card, device_node*, void* opaque) {
		PioneerEcamLeaseStateCall& call = *static_cast<PioneerEcamLeaseStateCall*>(opaque);
		PioneerPinnedNodes::Ref root(call.dm, call.dm->get_parent_node(card));
		if (root.node == NULL) return B_ENTRY_NOT_FOUND;
		PioneerPinnedNodes::Ref controller(call.dm, call.dm->get_parent_node(root.node));
		if (controller.node == NULL) return B_ENTRY_NOT_FOUND;
		driver_module_info* driver = NULL; void* cookie = NULL;
		if (call.dm->get_driver(controller.node, &driver, &cookie) != B_OK || driver == NULL
			|| driver->info.name == NULL || strcmp(driver->info.name, "busses/pci/ecam/driver_v1") != 0
			|| cookie == NULL) return B_UNSUPPORTED;
		pioneer_sg2042_lease_contention_module_info* probe = NULL;
		status_t status = get_module(PIONEER_SG2042_LEASE_CONTENTION_MODULE, (module_info**)&probe);
		if (status != B_OK) return status;
		bool active = false; thread_id waiter = -1; int32 waiters = 0;
		if (probe != NULL && probe->state != NULL)
			call.result->stateStatus = probe->state(cookie, &active, &waiter, &waiters);
		put_module(PIONEER_SG2042_LEASE_CONTENTION_MODULE);
		call.result->active = active ? 1 : 0;
		call.result->waiter = waiter; call.result->waiters = waiters;
		return call.result->stateStatus;
	}
};
#endif
#endif
