/*
 * Distributed under the MIT License.
 * Read-only device-tree, PCI topology, host-window and translation audit.
 */
#include "PioneerPciProfile.h"
#include "../../busses/pci/ecam/Sg2042RootPolicy.h"

#include <ByteOrder.h>
#include <drivers/bus/PCI.h>
#include <drivers/bus/FDT.h>
#include <string.h>

namespace {

struct NodeRef {
	device_manager_info* manager;
	device_node* node;
	NodeRef(device_manager_info* dm, device_node* value) : manager(dm), node(value) {}
	~NodeRef() { if (node != NULL) manager->put_node(node); }
	NodeRef(const NodeRef&) = delete;
	NodeRef& operator=(const NodeRef&) = delete;
};

} // namespace

status_t
GetPioneerPciProfile(device_manager_info* dm, const pci_info& expected,
	tt_pioneer_profile& profile)
{
	profile = {};
	profile.version = 1;
	profile.card_bus = expected.bus;
	NodeRef tree(dm, dm->get_root_node());
	if (tree.node == NULL)
		return B_ENTRY_NOT_FOUND;
	device_attr cardAttrs[] = {
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},
		{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, {.ui16 = 0x1e52}},
		{B_DEVICE_ID, B_UINT16_TYPE, {.ui16 = 0xb140}},
		{}
	};
	device_node* cardNode = NULL;
	if (dm->find_child_node(tree.node, cardAttrs, &cardNode) != B_OK)
		return B_ENTRY_NOT_FOUND;
	NodeRef card(dm, cardNode);
	profile.stage = 1;
	pci_device_module_info* pci;
	pci_device* device;
	if (dm->get_driver(card.node, (driver_module_info**)&pci,
			(void**)&device) != B_OK
		|| strcmp(pci->info.info.name, "bus_managers/pci/driver_v1") != 0)
		return B_UNSUPPORTED;
	pci_info actual;
	pci->get_pci_info(device, &actual);
	uint8 localBus, slot, function;
	if (actual.vendor_id != expected.vendor_id || actual.device_id != expected.device_id
		|| actual.bus != expected.bus || actual.device != 0 || actual.function != 0
		|| dm->get_attr_uint8(card.node, B_PCI_DEVICE_BUS, &localBus, false) != B_OK
		|| dm->get_attr_uint8(card.node, B_PCI_DEVICE_DEVICE, &slot, false) != B_OK
		|| dm->get_attr_uint8(card.node, B_PCI_DEVICE_FUNCTION, &function, false) != B_OK
		|| localBus != 1 || slot != 0 || function != 0)
		return B_UNSUPPORTED;
	profile.stage = 2;
	NodeRef root(dm, dm->get_parent_node(card.node));
	if (root.node == NULL)
		return B_ENTRY_NOT_FOUND;
	unsigned peerCount = 0;
	device_node* peer = NULL;
	device_attr peerAttrs[] = {
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},
		{}
	};
	while (dm->find_child_node(root.node, peerAttrs, &peer) == B_OK) {
		if (++peerCount > 2) {
			dm->put_node(peer);
			return B_UNSUPPORTED;
		}
	}
	if (peerCount != 2)
		return B_UNSUPPORTED;
	device_attr rootAttrs[] = {
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},
		{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {.ui8 = 0}},
		{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {.ui8 = 0}},
		{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {.ui8 = 0}},
		{}
	};
	device_node* rootPortNode = NULL;
	if (dm->get_next_child_node(root.node, rootAttrs, &rootPortNode) != B_OK)
		return B_ENTRY_NOT_FOUND;
	NodeRef rootPort(dm, rootPortNode);
	pci_device_module_info* rootPCI;
	pci_device* rootDevice;
	if (dm->get_driver(rootPort.node, (driver_module_info**)&rootPCI,
			(void**)&rootDevice) != B_OK
		|| strcmp(rootPCI->info.info.name, "bus_managers/pci/driver_v1") != 0
		|| rootPCI->read_pci_config(rootDevice, 0, 4) != 0x20421e30
		|| (rootPCI->read_pci_config(rootDevice, 8, 4) >> 8) != 0x060400)
		return B_UNSUPPORTED;
	pci_info rootInfo;
	rootPCI->get_pci_info(rootDevice, &rootInfo);
	profile.root_bus = rootInfo.bus;
	profile.root_command = rootPCI->read_pci_config(rootDevice, 4, 2);
	profile.root_buses = rootPCI->read_pci_config(rootDevice, 0x18, 4);
	profile.root_prefetch = rootPCI->read_pci_config(rootDevice, 0x24, 4);
	profile.root_prefetch_base_high = rootPCI->read_pci_config(rootDevice, 0x28, 4);
	profile.root_prefetch_limit_high = rootPCI->read_pci_config(rootDevice, 0x2c, 4);
	profile.root_memory = rootPCI->read_pci_config(rootDevice, 0x20, 4);
	profile.stage = 3;
	NodeRef controller(dm, dm->get_parent_node(root.node));
	if (controller.node == NULL)
		return B_ENTRY_NOT_FOUND;
	pci_controller_module_info* host;
	void* hostDevice;
	if (dm->get_driver(controller.node, (driver_module_info**)&host,
			&hostDevice) != B_OK
		|| strcmp(host->info.info.name, "busses/pci/ecam/driver_v1") != 0)
		return B_UNSUPPORTED;
	NodeRef fdtNode(dm, dm->get_parent_node(controller.node));
	if (fdtNode.node == NULL)
		return B_ENTRY_NOT_FOUND;
	const char* bus;
	if (dm->get_attr_string(fdtNode.node, B_DEVICE_BUS, &bus, false) != B_OK
		|| strcmp(bus, "fdt") != 0)
		return B_UNSUPPORTED;
	fdt_device_module_info* fdt;
	fdt_device* fdtDevice;
	if (dm->get_driver(fdtNode.node, (driver_module_info**)&fdt,
			(void**)&fdtDevice) != B_OK
		|| strcmp(fdt->info.info.name, "bus_managers/fdt/driver_v1") != 0)
		return B_UNSUPPORTED;
	uint64 ignored;
	if (!fdt->get_reg(fdtDevice, 0, &profile.controller, &ignored)
		|| ignored != 0x2000000
		|| !fdt->get_reg(fdtDevice, 1, &profile.config, &profile.config_length)
		|| !Sg2042BlackholeRootRegistersMatch(profile.controller,
			profile.config, profile.config_length))
		return B_UNSUPPORTED;
	int length = 0;
	const uint32* buses = (const uint32*)fdt->get_prop(fdtDevice, "bus-range", &length);
	if (buses == NULL || length != 8)
		return B_UNSUPPORTED;
	uint32 firstBus = B_BENDIAN_TO_HOST_INT32(buses[0]);
	uint32 lastBus = B_BENDIAN_TO_HOST_INT32(buses[1]);
	length = 0;
	const void* tag = fdt->get_prop(fdtDevice, "haiku,pioneer-root-probe", &length);
	if (length < 0 || !Sg2042BlackholeRootRequested(firstBus, lastBus, tag, length))
		return B_UNSUPPORTED;
	profile.stage = 4;
	const uint64 hosts[] = {0x4810800000, 0x4890000000, 0x48c0000000, 0x4900000000, 0x4b00000000};
	const uint64 addresses[] = {0x800000, 0x90000000, 0xc0000000, 0x4900000000, 0x4b00000000};
	const uint64 sizes[] = {0x400000, 0x30000000, 0x10000000, 0x200000000, 0x100000000};
	const uint32 flags[] = {0, PCI_address_prefetchable, 0,
		PCI_address_type_64 | PCI_address_prefetchable, PCI_address_type_64};
	for (unsigned i = 0; i < TT_PIONEER_RANGE_COUNT; i++) {
		pci_resource_range range;
		if (host->get_range(hostDevice, i, &range) != B_OK)
			return B_UNSUPPORTED;
		profile.ranges[i] = {range.host_address, range.pci_address, range.size,
			range.type, range.address_type};
		if (range.type != (i == 0 ? B_IO_PORT : B_IO_MEMORY) || range.host_address != hosts[i]
			|| range.pci_address != addresses[i] || range.size != sizes[i]
			|| range.address_type != flags[i])
			return B_UNSUPPORTED;
	}
	pci_resource_range extra;
	if (host->get_range(hostDevice, TT_PIONEER_RANGE_COUNT, &extra) == B_OK)
		return B_UNSUPPORTED;
	profile.stage = 5;
	profile.bar0_host_start = pci->ram_address(device, 0x4900000000);
	profile.bar0_host_end = pci->ram_address(device, 0x491fffffff);
	profile.bar2_host_start = pci->ram_address(device, 0x4920000000);
	profile.bar2_host_end = pci->ram_address(device, 0x49200fffff);
	if (profile.bar0_host_start != 0x4900000000 || profile.bar0_host_end != 0x491fffffff
		|| profile.bar2_host_start != 0x4920000000 || profile.bar2_host_end != 0x49200fffff)
		return B_UNSUPPORTED;
	profile.stage = 6;
	profile.ready = 1;
	return B_OK;
}
