/* Distributed under the MIT License. */
#ifndef PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE_H
#define PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE_H

#include <device_manager.h>

#define PIONEER_DEVICE_MANAGER_RETAINED_PIN_MODULE "system/device_manager/pioneer_retained_driver_pin/v1"

// Retains an initialized node and its ancestors across calls. The caller must
// already hold a node reference while acquiring, keep its owner identity and
// module reference alive, and serialize its own recovery transaction. This
// is a software lifetime reference: removal can still invalidate the device,
// and it grants no hardware, DMA, firmware, or reset ownership.
typedef status_t (*pioneer_dm_retained_pin_operation)(void* argument);
struct pioneer_dm_retained_pin_module_info {
	module_info info;
	status_t (*acquire)(device_node* node, const void* owner, uint64 session, uint64* pin);
	status_t (*run)(uint64 pin, const void* owner, uint64 session,
		pioneer_dm_retained_pin_operation operation, void* argument, status_t* operationStatus);
	// releaseAuthorized is supplied only after the caller has retired its
	// independent hardware and controller-lease obligations.
	status_t (*release)(uint64 pin, const void* owner, uint64 session, bool releaseAuthorized);
};
#endif
