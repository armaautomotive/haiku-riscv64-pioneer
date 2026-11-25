/* Distributed under the MIT License. Simulates the device-manager tree. */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h"
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
using status_t=int;
enum {B_OK=0,B_ENTRY_NOT_FOUND=-1,B_UNSUPPORTED=-2,B_BAD_VALUE=-3,B_ERROR=-4};
enum {B_STRING_TYPE=1,B_UINT16_TYPE,B_UINT8_TYPE,B_IO_PORT,B_IO_MEMORY};
enum {PCI_address_prefetchable=1,PCI_address_type_64=2};
#define B_DEVICE_BUS "bus"
#define B_DEVICE_VENDOR_ID "vendor"
#define B_DEVICE_ID "id"
#define B_PCI_DEVICE_BUS "pci_bus"
#define B_PCI_DEVICE_DEVICE "pci_device"
#define B_PCI_DEVICE_FUNCTION "pci_function"
#define B_BENDIAN_TO_HOST_INT32(x) (x)
#define dprintf(...) ((void)0)
struct device_node { int id,refs=0; };
struct device_attr { const char* name; int type; union {const char* string;uint16 ui16;uint8 ui8;}; };
struct pci_info { uint16 vendor_id,device_id;uint8 bus,device,function; };
struct pci_device {int id;}; struct fdt_device {};
struct pci_resource_range {uint64 host_address,pci_address,size;uint32 type,address_type;};
struct driver_module_info {};
struct module_name { const char* name; };struct nested {module_name info;};
struct pci_device_module_info {nested info;void (*get_pci_info)(pci_device*,pci_info*);
 uint32 (*read_pci_config)(pci_device*,uint16,uint8);uint64 (*ram_address)(pci_device*,uint64);};
struct pci_controller_module_info {nested info;status_t (*get_range)(void*,uint32,pci_resource_range*);};
struct fdt_device_module_info {nested info;bool (*get_reg)(fdt_device*,int,uint64*,uint64*);
 const void* (*get_prop)(fdt_device*,const char*,int*);};
struct device_manager_info {device_node* (*get_root_node)();status_t (*find_child_node)(device_node*,const device_attr*,device_node**);
 status_t (*get_driver)(device_node*,driver_module_info**,void**);status_t (*get_attr_uint8)(device_node*,const char*,uint8*,bool);
 device_node* (*get_parent_node)(device_node*);void (*put_node)(device_node*);
 status_t (*get_next_child_node)(device_node*,const device_attr*,device_node**);
 status_t (*get_attr_string)(device_node*,const char*,const char**,bool);};
using pioneer_pci_controller_operation=status_t (*)(void*,void*);
static device_node nodes[5]={{0},{1},{2},{3},{4}}; // tree, card, root bus, controller, fdt
static device_node port{5};static pci_device cardDevice{1},rootDevice{5};static fdt_device fdtDevice;
static int hostCookie,callbackCalls=0;static bool badFdt=false;
static device_node* acquire(device_node* n){++n->refs;return n;}
static void put(device_node* n){assert(n->refs>0);--n->refs;}
static device_node* tree(){return acquire(&nodes[0]);}
static device_node* parent(device_node* n){return acquire(n->id==1?&nodes[2]:n->id==2?&nodes[3]:&nodes[4]);}
static status_t find(device_node* n,const device_attr*,device_node** out){
 if(n->id==0){*out=acquire(&nodes[1]);return B_OK;}
 assert(n->id==2);if(!*out){*out=acquire(&port);return B_OK;}
 if(*out==&port){put(*out);*out=acquire(&nodes[1]);return B_OK;}
 put(*out);return B_ENTRY_NOT_FOUND;
}
static status_t next(device_node* n,const device_attr*,device_node** out){assert(n->id==2);*out=acquire(&port);return B_OK;}
static void pciInfo(pci_device* d,pci_info* p){*p=d->id==1?pci_info{0x1e52,0xb140,17,0,0}:pci_info{0x1e30,0x2042,16,0,0};}
static uint32 readConfig(pci_device*,uint16 offset,uint8){if(offset==0)return 0x20421e30;if(offset==8)return 0x06040000;return 0;}
static uint64 ram(pci_device*,uint64 a){return a;}
static status_t range(void*,uint32 i,pci_resource_range* r){
 const uint64 h[]={0x4810800000ULL,0x4890000000ULL,0x48c0000000ULL,0x4900000000ULL,0x4b00000000ULL};
 const uint64 p[]={0x800000,0x90000000,0xc0000000,0x4900000000ULL,0x4b00000000ULL};
 const uint64 s[]={0x400000,0x30000000,0x10000000,0x200000000ULL,0x100000000ULL};
 const uint32 f[]={0,PCI_address_prefetchable,0,PCI_address_type_64|PCI_address_prefetchable,PCI_address_type_64};
 if(i>=5)return B_ENTRY_NOT_FOUND;*r={h[i],p[i],s[i],i==0?B_IO_PORT:B_IO_MEMORY,f[i]};return B_OK;
}
static bool reg(fdt_device*,int i,uint64* address,uint64* size){*address=i?0x4800000000ULL:0x7062000000ULL;*size=i?0x1000:0x2000000;return true;}
static const void* prop(fdt_device*,const char* name,int* length){static uint32 buses[]={0x80,0xbf};static uint8 tag[]={0x1e,0x52,0xb1,0x40};
 if(!strcmp(name,"bus-range")){*length=8;return buses;}*length=badFdt?0:4;return tag;}
static pci_device_module_info pci={{ {"bus_managers/pci/driver_v1"}},pciInfo,readConfig,ram};
static pci_controller_module_info controller={{{"busses/pci/ecam/driver_v1"}},range};
static fdt_device_module_info fdt={{{"bus_managers/fdt/driver_v1"}},reg,prop};
static status_t driver(device_node* n,driver_module_info** m,void** cookie){
 if(n->id==1||n->id==5){*m=(driver_module_info*)&pci;*cookie=n->id==1?(void*)&cardDevice:(void*)&rootDevice;}
 else if(n->id==3){*m=(driver_module_info*)&controller;*cookie=&hostCookie;}
 else {*m=(driver_module_info*)&fdt;*cookie=&fdtDevice;}return B_OK;
}
static status_t attr8(device_node*,const char* name,uint8* out,bool){*out=!strcmp(name,B_PCI_DEVICE_BUS)?1:0;return B_OK;}
static status_t attrString(device_node*,const char*,const char** out,bool){*out="fdt";return B_OK;}
static device_manager_info manager{tree,find,driver,attr8,parent,put,next,attrString};
#include "pioneer_pci_profile_actual.inc"
static status_t callback(void* cookie,void*){
 assert(cookie==&hostCookie && nodes[0].refs && nodes[1].refs && nodes[2].refs && nodes[3].refs && nodes[4].refs && port.refs);
 ++callbackCalls;return B_ERROR;
}
static void released(){for(auto& n:nodes)assert(n.refs==0);assert(port.refs==0);}
int main(){pci_info expected{0x1e52,0xb140,17,0,0};tt_pioneer_profile profile{};
 assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_ERROR);
 assert(callbackCalls==1 && profile.ready && profile.stage==6);released();
 assert(GetPioneerPciProfile(&manager,expected,profile)==B_OK && callbackCalls==1);released();
 badFdt=true;assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_UNSUPPORTED);
 assert(callbackCalls==1 && !profile.ready);released();
 assert(WithPioneerPciController(nullptr,expected,profile,callback,nullptr)==B_BAD_VALUE);
 assert(WithPioneerPciController(&manager,expected,profile,nullptr,nullptr)==B_BAD_VALUE);released();
 std::puts("PASS actual PCI profile controller callback holds tree nodes and rejects mismatched FDT; simulated");
}
