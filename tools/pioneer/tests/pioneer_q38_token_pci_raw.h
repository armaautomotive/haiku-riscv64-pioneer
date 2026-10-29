/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_PCI_RAW_TEST_H
#define PIONEER_Q38_TOKEN_PCI_RAW_TEST_H
#if !defined(PIONEER_Q38_TOKEN_PCI_RAW_TEST) || PIONEER_Q38_TOKEN_PCI_RAW_TEST != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) \
 || (defined(_KERNEL_MODE) && (!defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1))
#error Explicit native PCI OS-endpoint TEST required
#endif
#include <cassert>
#include <cstring>
#pragma push_macro("_KERNEL_MODE")
#undef _KERNEL_MODE
#include "../../../src/add-ons/kernel/drivers/misc/BlackholePioneerMmioProbe.h"
#pragma pop_macro("_KERNEL_MODE")
#define PIONEER_MMIO_PROBE_MODEL_ONLY
namespace TokenPciRawModel {
#include "pioneer_mmio_probe_test.cpp"
}
#undef PIONEER_MMIO_PROBE_MODEL_ONLY
struct TokenPciRaw:TokenPciRawModel::MockIO {
 uint32_t pcieStatus[2]={};unsigned events=0;
 void* observer=nullptr;void (*after)(void*,bool,unsigned,unsigned,bool)=nullptr;
 void Event(bool root,unsigned offset,unsigned width,bool write){events++;if(after)after(observer,root,offset,width,write);}
 uint32_t CardRead(unsigned offset,unsigned width){
  uint32_t value;
  if(offset==0x60 && width==2)value=0x7001;
  else if(offset==0x70 && width==2)value=0x10;
  else if(offset==0x72 && width==2)value=2;
  else if(offset==0x7a && width==2)value=pcieStatus[0];
  else value=MockIO::CardRead(offset,width);
  Event(false,offset,width,false);return value;
 }
 uint32_t RootRead(unsigned offset,unsigned width){
  uint32_t value;
  if(offset==0x34 && width==1)value=0x70;
  else if(offset==0x70 && width==2)value=0x10;
  else if(offset==0x72 && width==2)value=2;
  else if(offset==0x7a && width==2)value=pcieStatus[1];
  else if((offset==0x10 || offset==0x14 || offset==0x38) && width==4)value=0;
  else value=MockIO::RootRead(offset,width);
  Event(true,offset,width,false);return value;
 }
 void CardWrite(unsigned offset,unsigned width,uint32_t value){MockIO::CardWrite(offset,width,value);Event(false,offset,width,true);}
 void RootWrite(unsigned offset,unsigned width,uint32_t value){MockIO::RootWrite(offset,width,value);Event(true,offset,width,true);}
}; // raw PCI bytes only;actual Blackhole BEGIN/controller execute separately
#endif
