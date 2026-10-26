/* SPDX-License-Identifier: MIT */
#include "PioneerQ38DensePrepareImageRole.h"
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQ38DensePrepare.h"
#include "PioneerQ38DensePrepareLayout.h"
extern "C" void scalar_fault_park() __attribute__((noreturn));
extern "C" unsigned scalar_main(){
 namespace L=PioneerQ38DensePrepareLayout;
 const auto* d=reinterpret_cast<const uint32_t*>(L::Header);
 if(!L::Shape(d))return 0;
 // LOCAL component only. No fake KV commit,producer,NoC/CSR or driver role.
 if(!PioneerQ38DensePrepare::Run(reinterpret_cast<const float*>(L::Qg),reinterpret_cast<const float*>(L::Key),
  reinterpret_cast<const float*>(L::Value),reinterpret_cast<const float*>(L::QWeight),reinterpret_cast<const float*>(L::KWeight),
  reinterpret_cast<const float*>(L::Rope),reinterpret_cast<float*>(L::Query),reinterpret_cast<float*>(L::Gate),
  reinterpret_cast<uint32_t*>(L::PackedK),reinterpret_cast<uint32_t*>(L::PackedV)))scalar_fault_park();
 return 1536;
}
