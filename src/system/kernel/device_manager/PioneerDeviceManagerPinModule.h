/* Distributed under the MIT License. */
#ifndef PIONEER_DEVICE_MANAGER_PIN_MODULE_H
#define PIONEER_DEVICE_MANAGER_PIN_MODULE_H

#include <device_manager.h>

#define PIONEER_DEVICE_MANAGER_PIN_MODULE "system/device_manager/pioneer_driver_pin/v1"

// A synchronous driver-initialization reference. The caller must hold an
// existing reference to node for the whole call. A successful pin retains the
// initialized node and its ancestors, but does not prevent device_removed()
// notifications or prove that any physical device remains accessible.
typedef status_t (*pioneer_dm_pinned_operation)(void* argument);
struct pioneer_dm_driver_pin_module_info {
	module_info info;
	status_t (*run)(device_node* node, pioneer_dm_pinned_operation operation,
		void* argument, status_t* operationStatus);
};
#endif
