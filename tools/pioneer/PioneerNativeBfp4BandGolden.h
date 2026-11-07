/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_BAND_GOLDEN_H
#define PIONEER_NATIVE_BFP4_BAND_GOLDEN_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Synthetic BFP4 golden words are an offline diagnostic, not a kernel or inference path
#endif
#include <stdint.h>
// Frozen documented-arithmetic predictions for PioneerNativeBfp4BandFixture.
// Independently checked against Python exact-rational arithmetic and actual
// interpreted RV32 bytes. They are not a silicon measurement or model output.
static inline const uint32_t* PioneerNativeBfp4BandGolden(unsigned width){
 static const uint32_t k5120[32]={
  0xc2535b06,0x41de875c,0x44128cdc,0x436e5763,0xc3c16a24,0xc384df85,0xc280e0c9,0xc3aee995,
  0x42aa6787,0x4433d8e2,0x4139df40,0xc41212ba,0x412d4900,0xc380d876,0xc350a34d,0xc31c179c,
  0x442fc7cb,0xc277dfb2,0xc3eb8d5c,0xc3291aff,0x41afdb54,0xc2c63a43,0x428c06c0,0x4448740e,
  0xc2a1eb11,0xc3da7471,0xc286fff2,0x3e39e600,0xc275b6a8,0xc300ffe5,0x43f299cd,0xc3a038ba};
 static const uint32_t k6144[32]={
  0xc2c3f34e,0x43471e60,0x43595896,0x4347c46e,0xc37d2b36,0xc349a4cd,0x42fce18e,0xc3b8d9f3,
  0x439dec1c,0x43ffcae6,0x42894d41,0xc3df2370,0xc18a2b48,0xc281425f,0xc3869462,0x431a7629,
  0x44047d8d,0xc2b90d47,0xc2c2e606,0xc38fd655,0x43118c95,0xc328d838,0x42f33e95,0x4407f19b,
  0xc1a9ae1c,0xc22e5fd6,0xc314f2db,0x43566004,0xc26c8688,0xc2f0558a,0x43adf66a,0xc3a4a69d};
 static const uint32_t k17408[32]={
  0xc2a04b26,0xc24cc142,0x436fca19,0x443094a3,0xc324ed3f,0xc40cd84c,0xc41c42a6,0xc38a8542,
  0xbe835b00,0x43e13a0b,0x441022bc,0xc35c5ec3,0xc2c5435f,0xc315e8b7,0x428a8551,0xc30fb9d3,
  0x440d5c8d,0x43d46d3c,0xc290c9f0,0xc3fff562,0x41d27ca4,0xc1ee90cc,0xc39a3b1e,0x44249f10,
  0x440c87ca,0x42ef0fac,0xc3959cf5,0x435efe04,0xc31b987c,0xc3e320f4,0x4374c618,0x43381032};
 return width==5120?k5120:width==6144?k6144:width==17408?k17408:nullptr;
}
#endif
