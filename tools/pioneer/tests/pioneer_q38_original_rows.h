/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error Original row transport is TEST data only,not device/native/Work authority
#endif
#ifndef PIONEER_Q38_ORIGINAL_ROWS_H
#define PIONEER_Q38_ORIGINAL_ROWS_H
#include <cstdint>
#include <string>
struct Q38OriginalTensor {
 std::string name;unsigned type=0,rank=0;uint64_t inputs=0,outputs=0,bytes=0,offset=0;
 bool operator==(const Q38OriginalTensor& b)const{return name==b.name && type==b.type && rank==b.rank && inputs==b.inputs && outputs==b.outputs && bytes==b.bytes && offset==b.offset;}
};
// Only immutable source descriptors and original packed/F32 learned bytes.
// No tiling,CPU model products,accelerator callback or readiness setter.
// TT_PIONEER_Q38_WORK_MODE also selects the tensor arithmetic policy in the
// offline instruction harness. RAW data transport may enter that harness;
// the separate CPU-PRODUCT header still rejects this flag,even defined0.
struct Q38OriginalRows {
 virtual ~Q38OriginalRows()=default;
 virtual bool Ready()const=0;virtual uint64_t Epoch()const=0;
 virtual bool Describe(unsigned,Q38OriginalTensor&)const=0;
 virtual bool ReadOriginal(unsigned,uint64_t,unsigned,void*,unsigned)=0;
};
#endif
