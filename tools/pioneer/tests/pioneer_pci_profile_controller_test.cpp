/* Distributed under the MIT License. Simulates the device-manager tree. */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciProfile.h"
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h"
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
using status_t=int;
enum {B_OK=0,B_ENTRY_NOT_FOUND=-1,B_UNSUPPORTED=-2,B_BAD_VALUE=-3,B_ERROR=-4,B_NOT_ALLOWED=-5,
 B_BUSY=-6,B_DEV_NOT_READY=-7,B_NO_MEMORY=-8};
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
constexpr unsigned NODE_FLAG_DEVICE_REMOVED=1;
struct device_node {
 int id,refs=0,pins=0;
 bool IsRegistered() const {return true;}
 bool IsInitialized() const {return true;}
 unsigned Flags() const {return 0;}
 status_t InitDriver(){++pins;return B_OK;}
 bool UninitDriver(){assert(pins>0);--pins;return false;}
};
static std::recursive_mutex sLock;
struct RecursiveLocker {std::unique_lock<std::recursive_mutex> lock;explicit RecursiveLocker(std::recursive_mutex& m):lock(m) {}};
struct device_attr { const char* name; int type; union {const char* string;uint16 ui16;uint8 ui8;}; };
struct pci_info { uint16 vendor_id,device_id;uint8 bus,device,function; };
struct pci_device {int id;}; struct fdt_device {};
struct pci_resource_range {uint64 host_address,pci_address,size;uint32 type,address_type;};
struct module_name { const char* name; };struct nested {module_name info;};
struct driver_module_info {module_name info;};
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
using pioneer_pci_controller_nodes_operation=status_t (*)(void*,device_node*,device_node*,void*);
using pioneer_dm_pinned_operation=status_t (*)(void*);
using pioneer_dm_retained_pin_operation=status_t (*)(void*);
struct module_info {};
struct pioneer_dm_driver_pin_module_info {module_info info;status_t (*run)(device_node*,pioneer_dm_pinned_operation,void*,status_t*);};
#define PIONEER_DEVICE_MANAGER_PIN_MODULE "test-pin"
static int pinDepth=0,pinRefs=0;static bool pinUnavailable=false,portPinDenied=false;
#include "pioneer_dm_driver_pin.inc"
#include "pioneer_dm_retained_pin_service.inc"
static status_t pinRun(device_node* n,pioneer_dm_pinned_operation operation,void* argument,status_t* result){
 assert(n && (n->id==1 || n->id==5) && n->refs>0);if(portPinDenied && n->id==5)return B_NOT_ALLOWED;++pinDepth;status_t status=pioneer_run_with_initialized_driver(n,operation,argument,result);--pinDepth;return status;
}
static pioneer_dm_driver_pin_module_info pinService{{},pinRun};
static status_t get_module(const char*,module_info** out){if(pinUnavailable)return B_ERROR;*out=(module_info*)&pinService;++pinRefs;return B_OK;}
static void put_module(const char*){assert(pinRefs==1);--pinRefs;}
static device_node nodes[5]={{0},{1},{2},{3},{4}}; // tree, card, root bus, controller, fdt
static device_node port{5},alternatePort{6};static bool replacePort=false;static int portSearches=0;static pci_device cardDevice{1},rootDevice{5};static fdt_device fdtDevice;
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
static status_t next(device_node* n,const device_attr*,device_node** out){assert(n->id==2);++portSearches;*out=acquire(replacePort && portSearches==2?&alternatePort:&port);return B_OK;}
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
static pci_device_module_info wrongPci={{ {"wrong/driver"}},pciInfo,readConfig,ram};
static bool wrongMetadata=false;
static pci_controller_module_info controller={{{"busses/pci/ecam/driver_v1"}},range};
static fdt_device_module_info fdt={{{"bus_managers/fdt/driver_v1"}},reg,prop};
static status_t driver(device_node* n,driver_module_info** m,void** cookie){
 if(n->id==1||n->id==5){*m=(driver_module_info*)(wrongMetadata && n->id==5?&wrongPci:&pci);*cookie=n->id==1?(void*)&cardDevice:(void*)&rootDevice;}
 else if(n->id==3){*m=(driver_module_info*)&controller;*cookie=&hostCookie;}
 else {*m=(driver_module_info*)&fdt;*cookie=&fdtDevice;}return B_OK;
}
static status_t attr8(device_node*,const char* name,uint8* out,bool){*out=!strcmp(name,B_PCI_DEVICE_BUS)?1:0;return B_OK;}
static status_t attrString(device_node*,const char*,const char** out,bool){*out="fdt";return B_OK;}
static device_manager_info manager{tree,find,driver,attr8,parent,put,next,attrString};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciPinnedNodes.h"
struct pioneer_pci_pin_probe_result {uint32 version;status_t managerStatus,pinStatus,cardDriverStatus,portDriverStatus;uint32 callbacks,matchingDrivers;};
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciPinProbeLogic.h"
#include "pioneer_pci_profile_actual.inc"
static status_t callback(void* cookie,void*){
 assert(pinDepth==2 && nodes[1].pins==1 && port.pins==1 && cookie==&hostCookie && nodes[0].refs && nodes[1].refs && nodes[2].refs && nodes[3].refs && nodes[4].refs && port.refs);
 ++callbackCalls;return B_ERROR;
}
static int retainedOwner;
static uint64 retainedCard=0,retainedPort=0;
static status_t nodesCallback(void* cookie,device_node* card,device_node* root,void*) {
 assert(cookie==&hostCookie && card==&nodes[1] && root==&port);
 assert(pinDepth==2 && card->pins==1 && root->pins==1 && card->refs && root->refs);
 assert(pioneer_acquire_retained_pin(card,&retainedOwner,1,&retainedCard)==B_OK);
 assert(pioneer_acquire_retained_pin(root,&retainedOwner,1,&retainedPort)==B_OK);
 assert(card->pins==2 && root->pins==2);
 ++callbackCalls;return B_ERROR;
}
static status_t retainedCallback(void*) {return B_OK;}
static void released(){for(auto& n:nodes)assert(n.refs==0);assert(port.refs==0 && alternatePort.refs==0);}
int main(){pci_info expected{0x1e52,0xb140,17,0,0};tt_pioneer_profile profile{};
 pioneer_pci_pin_probe_result probe{};PioneerPciPinProbeCall call{&manager,&probe};
 assert(PioneerWithPinnedPciNodes(&manager,PioneerPciPinProbeCall::Check,&call)==B_OK);
 assert(probe.callbacks==1 && probe.matchingDrivers==1 && !pinDepth && !pinRefs);released();
 wrongMetadata=true;probe={};
 assert(PioneerWithPinnedPciNodes(&manager,PioneerPciPinProbeCall::Check,&call)==B_UNSUPPORTED);
 assert(probe.callbacks==1 && !probe.matchingDrivers && !pinDepth && !pinRefs);released();
 wrongMetadata=false;
 assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_ERROR);
 assert(callbackCalls==1 && profile.ready && profile.stage==6);released();
 assert(WithPioneerPciControllerNodes(&manager,expected,profile,nodesCallback,nullptr)==B_ERROR);
 assert(callbackCalls==2 && profile.ready && profile.stage==6);released();
 assert(nodes[1].pins==1 && port.pins==1 && retainedCard && retainedPort);
 status_t retainedResult=B_ERROR;
 assert(pioneer_run_retained_pin(retainedCard,&retainedOwner,1,retainedCallback,nullptr,&retainedResult)==B_OK);
 assert(retainedResult==B_OK);
 assert(pioneer_release_retained_pin(retainedPort,&retainedOwner,1,true)==B_OK);
 assert(pioneer_release_retained_pin(retainedCard,&retainedOwner,1,true)==B_OK);
 assert(!nodes[1].pins && !port.pins);
 assert(GetPioneerPciProfile(&manager,expected,profile)==B_OK && callbackCalls==2);released();
 badFdt=true;assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_UNSUPPORTED);
 assert(callbackCalls==2 && !profile.ready);released();
 assert(WithPioneerPciControllerNodes(&manager,expected,profile,nodesCallback,nullptr)==B_UNSUPPORTED);
 assert(callbackCalls==2 && !profile.ready);released();
 badFdt=false;replacePort=true;portSearches=0;
 assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_NOT_ALLOWED);
 assert(callbackCalls==2 && !profile.ready && nodes[1].pins==0 && port.pins==0);released();
 portSearches=0;
 assert(WithPioneerPciControllerNodes(&manager,expected,profile,nodesCallback,nullptr)==B_NOT_ALLOWED);
 assert(callbackCalls==2 && !profile.ready && nodes[1].pins==0 && port.pins==0);released();
 replacePort=false;portPinDenied=true;
 assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_NOT_ALLOWED);
 assert(!profile.ready && callbackCalls==2 && pinDepth==0 && pinRefs==0 && nodes[1].pins==0 && port.pins==0);released();
 portPinDenied=false;pinUnavailable=true;assert(WithPioneerPciController(&manager,expected,profile,callback,nullptr)==B_ERROR);
 assert(!profile.ready && callbackCalls==2 && pinDepth==0 && pinRefs==0 && nodes[1].pins==0 && port.pins==0);released();
 assert(WithPioneerPciController(nullptr,expected,profile,callback,nullptr)==B_BAD_VALUE);
 assert(WithPioneerPciController(&manager,expected,profile,nullptr,nullptr)==B_BAD_VALUE);released();
 assert(WithPioneerPciControllerNodes(nullptr,expected,profile,nodesCallback,nullptr)==B_BAD_VALUE);
 assert(WithPioneerPciControllerNodes(&manager,expected,profile,nullptr,nullptr)==B_BAD_VALUE);released();
 std::puts("PASS actual profile and device-manager pin compose over simulated nodes and reject mismatched FDT");
}
