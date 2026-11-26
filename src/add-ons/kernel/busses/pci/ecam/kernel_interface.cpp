/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "ECAMPCIController.h"
#ifdef PIONEER_SG2042_RECOVERY_WITNESS
#include "PioneerSg2042RecoveryModule.h"
#endif
#ifdef PIONEER_SG2042_RECOVERY_QUALIFICATION
#include "PioneerSg2042LeaseProbeModule.h"
#endif


device_manager_info* gDeviceManager;
pci_module_info* gPCI;


pci_controller_module_info gPciControllerDriver = {
	.info = {
		.info = {
			.name = ECAM_PCI_DRIVER_MODULE_NAME,
		},
		.supports_device = ECAMPCIController::SupportsDevice,
		.register_device = ECAMPCIController::RegisterDevice,
		.init_driver = [](device_node* node, void** driverCookie) {
			return ECAMPCIController::InitDriver(node, *(ECAMPCIController**)driverCookie);
		},
		.uninit_driver = [](void* driverCookie) {
			return static_cast<ECAMPCIController*>(driverCookie)->UninitDriver();
		},
	},
	.read_pci_config = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32* value) {
		return static_cast<ECAMPCIController*>(cookie)
			->ReadConfig(bus, device, function, offset, size, *value);
	},
	.write_pci_config = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) {
		return static_cast<ECAMPCIController*>(cookie)
			->WriteConfig(bus, device, function, offset, size, value);
	},
	.get_max_bus_devices = [](void* cookie, int32* count) {
		return static_cast<ECAMPCIController*>(cookie)->GetMaxBusDevices(*count);
	},
	.read_pci_irq = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8 *irq) {
		return static_cast<ECAMPCIController*>(cookie)->ReadIrq(bus, device, function, pin, *irq);
	},
	.write_pci_irq = [](void* cookie,
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8 irq) {
		return static_cast<ECAMPCIController*>(cookie)->WriteIrq(bus, device, function, pin, irq);
	},
	.get_range = [](void *cookie, uint32 index, pci_resource_range* range) {
		return static_cast<ECAMPCIController*>(cookie)->GetRange(index, range);
	},
	.finalize = [](void *cookie) {
		return static_cast<ECAMPCIController*>(cookie)->Finalize();
	}
};

#ifdef PIONEER_SG2042_RECOVERY_WITNESS
pioneer_sg2042_recovery_module_info gPioneerRecovery = {
	.info = { .name = PIONEER_SG2042_RECOVERY_MODULE },
	.run = [](void* cookie, pioneer_sg2042_recovery_operation operation,
		void* argument, status_t* operationStatus) {
		#ifdef PIONEER_SG2042_RECOVERY_GATEWAY_WITNESS
		if (cookie == NULL)
			return B_BAD_VALUE;
		return static_cast<ECAMPCIController*>(cookie)->RunPioneerRecovery(
			operation, argument, operationStatus);
		#else
		(void)cookie; (void)operation; (void)argument; (void)operationStatus;
		return B_NOT_ALLOWED;
		#endif
	}
};
#endif

#ifdef PIONEER_SG2042_RECOVERY_QUALIFICATION
// PIONEER_SG2042_LEASE_PROBE_BEGIN
static status_t
pioneer_sg2042_lease_probe(void* cookie, uint32* callbacks, status_t* callbackStatus)
{
	if (cookie == NULL || callbacks == NULL || callbackStatus == NULL)
		return B_BAD_VALUE;
	*callbacks = 0;
	*callbackStatus = B_ERROR;
	return static_cast<ECAMPCIController*>(cookie)->RunPioneerRecovery(
		[](void* argument) -> status_t {
			++*static_cast<uint32*>(argument);
			return B_CANCELED;
		}, callbacks, callbackStatus);
}
// PIONEER_SG2042_LEASE_PROBE_END

pioneer_sg2042_lease_probe_module_info gPioneerLeaseProbe = {
	.info = { .name = PIONEER_SG2042_LEASE_PROBE_MODULE },
	.probe = pioneer_sg2042_lease_probe
};

pioneer_sg2042_lease_contention_module_info gPioneerLeaseContention = {
	.info = { .name = PIONEER_SG2042_LEASE_CONTENTION_MODULE },
	.hold = [](void* cookie, sem_id ready, sem_id release, status_t* callbackStatus) {
		if (cookie == NULL) return B_BAD_VALUE;
		return static_cast<ECAMPCIController*>(cookie)->HoldPioneerQualification(ready, release, callbackStatus);
	},
	.state = [](void* cookie, bool* active, thread_id* waiter, int32* waiters) {
		if (cookie == NULL) return B_BAD_VALUE;
		return static_cast<ECAMPCIController*>(cookie)->PioneerQualificationState(active, waiter, waiters);
	}
};
#endif


_EXPORT module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_PCI_MODULE_NAME, (module_info**)&gPCI },
	{}
};

_EXPORT module_info *modules[] = {
	(module_info *)&gPciControllerDriver,
	#ifdef PIONEER_SG2042_RECOVERY_WITNESS
	(module_info *)&gPioneerRecovery,
	#endif
	#ifdef PIONEER_SG2042_RECOVERY_QUALIFICATION
	(module_info *)&gPioneerLeaseProbe,
	(module_info *)&gPioneerLeaseContention,
	#endif
	NULL
};
