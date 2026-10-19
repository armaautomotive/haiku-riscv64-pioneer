/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_TEST_MODULE_STUB_H
#define PIONEER_TEST_MODULE_STUB_H
#include <cstdint>
#include <cstddef>
using uint32=uint32_t; using int32=int32_t; using uint64=uint64_t; using status_t=int; using sem_id=int;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_DEV_NOT_READY=-2,B_BUSY=-3,B_NOT_ALLOWED=-4;
constexpr uint32 B_KEEP_LOADED=1;
constexpr int B_MODULE_INIT=1,B_MODULE_UNINIT=2;
struct module_info { const char* name; uint32 flags; status_t (*std_ops)(int32,...); };
#endif
