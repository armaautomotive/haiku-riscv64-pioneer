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
#endif
