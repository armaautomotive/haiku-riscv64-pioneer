/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_SCRATCH_LAYOUT_H
#define PIONEER_Q38_FFN_SCRATCH_LAYOUT_H
#include "PioneerQwen38PackedPlan.h"
// OFFLINE DATA proposal ONLY. All offsets below are relative to ONE existing
// workspace fragment,NOT NoC/PCI/host addresses or allocated device handles.
// Does not initialize,claim,read,write,launch or free anything. Native use still
// requires an exclusive retained owner,authenticated source,immutable private
// range derivation,initialized/mutable interlock and actual hardware proofs.
struct PioneerQ38FfnScratchLayout {
 enum Slot {Input,QuantInput,Gate,Up,Activated,QuantActivated,Output,Slots};
 // The matrix wire uses native sign-magnitude16-bit operands containing
 // quantized int8 values,NOT signed int16 numbers or packed GGML Q8_K.
 enum Encoding {Float32,NativeOperand16AndFloat32Scales};
 struct Slice {uint32_t offset,bytes,elements,encoding,activationBytes,scaleBytes;};
 const PioneerQwen38Gguf* source=nullptr;
 uint64_t epoch=0;
 unsigned workspaceFragment=0,bank=0;
 uint32_t bytes=0;
 Slice slice[Slots]={};
 static bool Describe(const PioneerQwen38Gguf& model,const PioneerQwen38PackedPlan& plan,unsigned bank,
  PioneerQ38FfnScratchLayout& output) {
  if(bank>=7 || !model.Stable() || !model.Epoch() || !plan.Tiled() || !plan.MatchesSource(model))return false;
  const uint64_t epoch=model.Epoch();const auto& parts=plan.Parts();
  unsigned selected=UINT32_MAX;
  for(unsigned i=0;i<parts.size();i++) {
   const auto& p=parts[i];if(p.kind!=PIONEER_Q38_WORKSPACE || p.bank!=bank)continue;
   if(selected!=UINT32_MAX || p.layer!=64 || p.role || p.tensor!=851 || p.firstRow || p.rows!=1
    || p.rowBytes!=67108864 || p.bytes!=67108864 || p.allocated!=67108864
    || p.proposedAddress<PioneerQwen38PackedPlan::Base || (p.proposedAddress&4095)
    || uint64_t(p.proposedAddress)+p.allocated>uint64_t(PioneerQwen38PackedPlan::Base)+PioneerQwen38PackedPlan::Capacity)return false;
   selected=i;
  }
  if(selected==UINT32_MAX)return false;
  PioneerQ38FfnScratchLayout stage={};stage.source=&model;stage.epoch=epoch;stage.bank=bank;stage.workspaceFragment=selected;
  for(unsigned slot=0;slot<Slots;slot++) {
   const bool quantized=slot==QuantInput || slot==QuantActivated;
   const unsigned elements=slot==Gate || slot==Up || slot==Activated || slot==QuantActivated?17408:5120;
   auto& s=stage.slice[slot];s.offset=stage.bytes;s.elements=elements;
   s.encoding=quantized?NativeOperand16AndFloat32Scales:Float32;
   if(quantized){s.activationBytes=elements*2;s.scaleBytes=((elements/256)*4+63)&~63U;s.bytes=s.activationBytes+s.scaleBytes;}
   else s.bytes=elements*4;
   if(!s.bytes || (s.offset&63) || (s.bytes&63) || stage.bytes>parts[selected].bytes
    || s.bytes>parts[selected].bytes-stage.bytes)return false;
   stage.bytes+=s.bytes;
  }
  if(stage.bytes!=295360 || model.Epoch()!=epoch || !model.Stable() || !plan.MatchesSource(model))return false;
  output=stage;return true;
 }
};
#endif
