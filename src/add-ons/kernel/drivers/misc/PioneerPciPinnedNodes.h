/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_PCI_PINNED_NODES_H
#define PIONEER_PCI_PINNED_NODES_H

// Requires device_manager.h, PCI bus attribute definitions, and the pin
// module interface. This holds software lifetime references only: device
// removal, firmware and DMA must be excluded independently by the caller.
using pioneer_pinned_nodes_operation = status_t (*)(device_node*, device_node*, void*);

namespace PioneerPinnedNodes {
struct Ref {
	device_manager_info* manager;
	device_node* node;
	Ref(device_manager_info* m, device_node* n) : manager(m), node(n) {}
	~Ref() { if (node != NULL) manager->put_node(node); }
	Ref(const Ref&) = delete;
	Ref& operator=(const Ref&) = delete;
};
struct Call {
	device_manager_info* dm;
	pioneer_dm_driver_pin_module_info* pin;
	device_node* card;
	pioneer_pinned_nodes_operation operation;
	void* argument;
	device_node* port = NULL;
	static status_t RunPort(void* argument) {
		Call& call = *static_cast<Call*>(argument);
		return call.operation(call.card, call.port, call.argument);
	}
	static status_t RunCard(void* argument) {
		Call& call = *static_cast<Call*>(argument);
		Ref parent(call.dm, call.dm->get_parent_node(call.card));
		if (parent.node == NULL) return B_ENTRY_NOT_FOUND;
		device_attr attrs[] = {
			{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},
			{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {.ui8 = 0}},
			{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {.ui8 = 0}},
			{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {.ui8 = 0}},
			{}
		};
		device_node* node = NULL;
		if (call.dm->get_next_child_node(parent.node, attrs, &node) != B_OK)
			return B_ENTRY_NOT_FOUND;
		Ref port(call.dm, node);
		call.port = port.node;
		status_t result = B_ERROR;
		status_t status = call.pin->run(port.node, RunPort, &call, &result);
		call.port = NULL;
		return status == B_OK ? result : status;
	}
};
}

static status_t
PioneerWithPinnedPciNodes(device_manager_info* dm,
	pioneer_pinned_nodes_operation operation, void* argument)
{
	if (dm == NULL || operation == NULL) return B_BAD_VALUE;
	PioneerPinnedNodes::Ref tree(dm, dm->get_root_node());
	if (tree.node == NULL) return B_ENTRY_NOT_FOUND;
	device_attr attrs[] = {
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},
		{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, {.ui16 = 0x1e52}},
		{B_DEVICE_ID, B_UINT16_TYPE, {.ui16 = 0xb140}},
		{}
	};
	device_node* node = NULL;
	if (dm->find_child_node(tree.node, attrs, &node) != B_OK) return B_ENTRY_NOT_FOUND;
	PioneerPinnedNodes::Ref card(dm, node);
	pioneer_dm_driver_pin_module_info* pin = NULL;
	status_t status = get_module(PIONEER_DEVICE_MANAGER_PIN_MODULE, (module_info**)&pin);
	if (status != B_OK) return status;
	PioneerPinnedNodes::Call call{dm, pin, card.node, operation, argument};
	status_t result = B_ERROR;
	status = pin != NULL && pin->run != NULL
		? pin->run(card.node, PioneerPinnedNodes::Call::RunCard, &call, &result)
		: B_BAD_VALUE;
	put_module(PIONEER_DEVICE_MANAGER_PIN_MODULE);
	return status == B_OK ? result : status;
}
#endif
