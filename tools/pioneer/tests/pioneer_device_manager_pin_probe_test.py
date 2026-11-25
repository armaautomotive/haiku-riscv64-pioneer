#!/usr/bin/env python3
"""Exercise the actual card-independent probe with simulated kernel modules."""
from pathlib import Path
import os
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
with tempfile.TemporaryDirectory(prefix='q35-pin-probe-test-') as temporary:
    temp = Path(temporary)
    (temp / 'Drivers.h').write_text('''#pragma once
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
using int32=int32_t; using uint32=uint32_t; using status_t=int;
constexpr int B_CUR_DRIVER_API_VERSION=2, B_DEVICE_OP_CODES_END=9999;
constexpr int B_OK=0, B_NO_INIT=-1, B_CANCELED=-2, B_BAD_VALUE=-3, B_NOT_ALLOWED=-4;
struct device_hooks {status_t(*open)(const char*,uint32,void**);status_t(*close)(void*);
 status_t(*free)(void*);status_t(*control)(void*,uint32,void*,size_t);
 status_t(*read)(void*,off_t,void*,size_t*);status_t(*write)(void*,off_t,const void*,size_t*);
 void* select;void* deselect;void* readv;void* writev;};
''')
    (temp / 'KernelExport.h').write_text('''#pragma once
#include <Drivers.h>
struct module_info {const char* name;uint32 flags;void* ops;};
status_t get_module(const char*,module_info**);status_t put_module(const char*);
status_t user_memcpy(void*,const void*,size_t);
''')
    (temp / 'device_manager.h').write_text('''#pragma once
#include <KernelExport.h>
struct device_node {};
struct device_manager_info {device_node*(*get_root_node)();void(*put_node)(device_node*);};
#define B_DEVICE_MANAGER_MODULE_NAME "system/device_manager/v1"
''')
    harness = r'''
#include <cassert>
#include <cstring>
#include "SOURCE"
static device_node root;
static int pinRefs=0,dmRefs=0,nodes=0,mode=0;
static device_node* getroot(){if(mode==3)return nullptr;++nodes;return &root;}
static void putnode(device_node* n){assert(n==&root);--nodes;}
static status_t run(device_node* n,pioneer_dm_pinned_operation op,void* arg,status_t* out){
 if(!n || !op || !out)return B_BAD_VALUE;
 if(mode==4)return B_NOT_ALLOWED;
 *out=op(arg);return B_OK;
}
static pioneer_dm_driver_pin_module_info pin={{PIONEER_DEVICE_MANAGER_PIN_MODULE,0,nullptr},run};
static device_manager_info dm={getroot,putnode};
status_t get_module(const char* n,module_info** out){
 if(!strcmp(n,PIONEER_DEVICE_MANAGER_PIN_MODULE)) {if(mode==1)return B_NOT_ALLOWED;++pinRefs;*out=(module_info*)&pin;return B_OK;}
 assert(!strcmp(n,B_DEVICE_MANAGER_MODULE_NAME));if(mode==2)return B_NOT_ALLOWED;
 ++dmRefs;*out=(module_info*)&dm;return B_OK;
}
status_t put_module(const char* n){if(!strcmp(n,PIONEER_DEVICE_MANAGER_PIN_MODULE))--pinRefs;else --dmRefs;return B_OK;}
status_t user_memcpy(void* out,const void* in,size_t size){if(!out)return B_BAD_VALUE;memcpy(out,in,size);return B_OK;}
int main(){
 assert(init_hardware()==B_OK && init_driver()==B_OK);
 assert(find_device("misc/pioneer_dm_pin_probe") && !find_device("not-this-device"));
 pioneer_dm_pin_probe_result r={};
 assert(probe_control(nullptr,0,&r,sizeof(r))==B_BAD_VALUE);
 assert(probe_control(nullptr,PIONEER_DM_PIN_PROBE,&r,sizeof(r)-1)==B_BAD_VALUE);
 for(mode=0;mode<5;++mode){
  r={};assert(probe_control(nullptr,PIONEER_DM_PIN_PROBE,&r,sizeof(r))==B_OK);
  assert(r.version==1 && pinRefs==0 && dmRefs==0 && nodes==0);
  if(mode==0)assert(r.pinModuleStatus==B_OK && r.managerModuleStatus==B_OK && r.invalidStatus==B_BAD_VALUE
      && r.rootStatus==B_OK && r.callbackStatus==B_CANCELED && r.callbacks==1);
  if(mode==1)assert(r.pinModuleStatus==B_NOT_ALLOWED && r.managerModuleStatus==B_NO_INIT && r.callbacks==0);
  if(mode==2)assert(r.managerModuleStatus==B_NOT_ALLOWED && r.rootStatus==B_NO_INIT && r.callbacks==0);
  if(mode==3)assert(r.rootStatus==B_NO_INIT && r.callbacks==0);
  if(mode==4)assert(r.rootStatus==B_NOT_ALLOWED && r.callbacks==0);
 }
 assert(probe_control(nullptr,PIONEER_DM_PIN_PROBE,nullptr,sizeof(r))==B_BAD_VALUE);
 uninit_driver();
}
'''.replace('SOURCE', str(repo / 'src/add-ons/kernel/drivers/misc/pioneer_device_manager_pin_probe.cpp'))
    (temp / 'test.cpp').write_text(harness)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(temp), str(temp / 'test.cpp'),
                    '-o', str(temp / 'test')], check=True)
    subprocess.run([str(temp / 'test')], check=True)
print('PASS actual pin probe with simulated modules, missing roots and balanced references; no card')
