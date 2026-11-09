/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Qgw8LayerTraceAssembler.h"
#include "PioneerQ38Qgw8LayerTracePacket.h"
#ifndef PIONEER_Q38_QGW8_LAYER_TRACE_HANDOFF_H
#define PIONEER_Q38_QGW8_LAYER_TRACE_HANDOFF_H

// Offline packet/assembly bridge. Source key is only caller-supplied DATA;
// matching it does not authenticate an owner, FD, card or completed session.
class PioneerQ38Qgw8LayerTraceHandoff {
 using P=PioneerQ38Qgw8LayerTracePacket::Packet;
 using Request=PioneerQ38Qgw8LayerTracePacket::Request;
 const uint64_t key;PioneerQ38Qgw8LayerTraceAssembler assembly;Request pending={};bool waiting=false;
 bool ExternalSafe(const void* p,size_t bytes)const{
  const uintptr_t begin=reinterpret_cast<uintptr_t>(p),self=reinterpret_cast<uintptr_t>(this);
  return p && bytes && begin<=UINTPTR_MAX-bytes && self<=UINTPTR_MAX-sizeof(*this)
   && !(begin<self+sizeof(*this) && self<begin+bytes) && assembly.ExternalBufferSafe(p,bytes);
 }
public:
 explicit PioneerQ38Qgw8LayerTraceHandoff(uint64_t id,uint32_t workspace):key(id),assembly(id,workspace){}
 bool Failed()const{return assembly.Failed();}uint32_t Cursor()const{return assembly.Cursor();}
 bool Prepare(P& packet){
  namespace Q=PioneerQ38Qgw8LayerTracePacket;namespace G=PioneerQ38Qgw8LayerTraceGeometry;
  if(Failed() || Cursor()>=G::Chunks)return false;
  if(waiting || !ExternalSafe(&packet,sizeof(packet)))return assembly.Invalidate();
  const auto* bytes=reinterpret_cast<const uint8_t*>(&packet);
  for(size_t i=0;i<sizeof(packet);i++)if(bytes[i])return assembly.Invalidate();
  const uint32_t ordinal=Cursor(),part=ordinal%G::Parts;
  packet.request={Q::Magic,Q::Version,ordinal,ordinal/G::Parts,part,part?1024U:4096U,uint32_t(key),uint32_t(key>>32),{}};
  if(!Q::RequestShape(packet))return assembly.Invalidate();pending=packet.request;waiting=true;return true;
 }
 bool Accept(const P& packet){
  namespace Q=PioneerQ38Qgw8LayerTracePacket;
  if(Failed())return false;
  if(!waiting || !ExternalSafe(&packet,sizeof(packet)) || std::memcmp(&pending,&packet.request,sizeof(pending)) || !Q::ReplyShape(packet)
   || (uint64_t(pending.keyHigh)<<32|pending.keyLow)!=key)return assembly.Invalidate();
  if(!assembly.Append(key,pending.ordinal,packet.words,pending.words))return false;
  waiting=false;pending={};return true;
 }
 bool Copy(uint32_t* output,size_t words){
  return !waiting && words==size_t(64)*5120 && ExternalSafe(output,words*sizeof(uint32_t)) && assembly.Copy(output,words);
 }
};
#endif
