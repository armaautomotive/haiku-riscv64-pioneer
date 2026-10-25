#!/usr/bin/env python3
"""Execute ACTUAL closed ProbeControl prefix with routing-only TEST handlers."""
import os
import pathlib
import re
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
native=(kernel/'tenstorrent_pci_probe.cpp').read_text()
start=native.index('static status_t\nProbeControl(')
end=native.index('// PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE_ROUTING_END',start)
body=native[start:end]+'\n#endif\n}\n'
names=set(re.findall(r'\bTT_PCI_PROBE_[A-Z0-9_]+\b',body))
constants={}
for name in ('PioneerDriverReloadProtocol.h','PioneerQ38BankProtocol.h','PioneerQ38SpanProtocol.h',
             'PioneerQ38ResidentOutputValidationProtocol.h'):
    for symbol,value in re.findall(r'#define\s+(TT_PCI_PROBE_[A-Z0-9_]+)\s+\(B_DEVICE_OP_CODES_END\s*\+\s*(0x[0-9a-fA-F]+)\)',(kernel/name).read_text()):
        if symbol in names:
            assert symbol not in constants
            constants[symbol]=0x1000+int(value,16)
assert set(constants)==names and len(constants)==10 and len(set(constants.values()))==10
source=r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
using uint32=uint32_t;using status_t=int32_t;
enum {B_NOT_ALLOWED=-1};
#define TT_PIONEER_DRIVER_RELOAD 1
#define PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE 1
struct PioneerProbeCookie {uint32 magic=0x50313030;};
static unsigned currentUID=0,calls=0,last=0;static void* expectedData=nullptr;static size_t expectedLength=0;
static int geteuid(){return int(currentUID);}
static status_t Handler(PioneerProbeCookie* c,uint32 op,void* data,size_t length,int result){
 assert(c && c->magic==0x50313030 && data==expectedData && length==expectedLength);calls++;last=op;return result;
}
static status_t DriverReloadControl(PioneerProbeCookie* c,uint32 o,void* d,size_t n){return Handler(c,o,d,n,4);}
static status_t Q38ResidentOutputValidationControl(PioneerProbeCookie* c,uint32 o,void* d,size_t n){return Handler(c,o,d,n,3);}
static status_t Q38BankControl(PioneerProbeCookie* c,uint32 o,void* d,size_t n){return Handler(c,o,d,n,1);}
static status_t Q38SpanControl(PioneerProbeCookie* c,uint32 o,void* d,size_t n){return Handler(c,o,d,n,2);}
'''
source+='\n'.join('#define '+key+' '+str(value) for key,value in constants.items())+'\n'+body
source+=r'''
int main(){
 PioneerProbeCookie cookie;expectedData=&cookie;expectedLength=123;unsigned allowed=0;
 for(unsigned operation=0;operation<131072;operation++){
  calls=0;last=UINT32_MAX;int expected=B_NOT_ALLOWED;
  if(operation==TT_PCI_PROBE_RELOAD_STATUS || operation==TT_PCI_PROBE_RELOAD_QUIESCE)expected=4;
  if(operation==TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATION_SUPPORT || operation==TT_PCI_PROBE_Q38_RESIDENT_OUTPUT_VALIDATE)expected=3;
  if(operation==TT_PCI_PROBE_Q38_BANK_SUPPORT || operation==TT_PCI_PROBE_Q38_BANK_BEGIN || operation==TT_PCI_PROBE_Q38_BANK_END)expected=1;
  if(operation==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || operation==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || operation==TT_PCI_PROBE_Q38_SPAN_END)expected=2;
  assert(ProbeControl(&cookie,operation,expectedData,expectedLength)==expected);
  assert(calls==unsigned(expected!=B_NOT_ALLOWED));if(calls){assert(last==operation);allowed++;}
  calls=0;currentUID=1;assert(ProbeControl(&cookie,operation,expectedData,expectedLength)==B_NOT_ALLOWED && !calls);currentUID=0;
  cookie.magic=0;assert(ProbeControl(&cookie,operation,expectedData,expectedLength)==B_NOT_ALLOWED && !calls);cookie.magic=0x50313030;
  assert(ProbeControl(nullptr,operation,expectedData,expectedLength)==B_NOT_ALLOWED && !calls);
 }
 assert(allowed==10);
 puts("PASS ACTUAL DEBUG68 closed ProbeControl prefix/131072 operations/10exact allowed/root+cookie checks/handler arguments unchanged;handlers TEST routing tags ONLY,NO IO/native/source/model or execution proof");
}
'''
with tempfile.TemporaryDirectory(prefix='pioneer-output-debug68-routing-',dir='/private/tmp') as temporary:
    binary=pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-x','c++','-','-o',str(binary)],input=source,text=True,check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
