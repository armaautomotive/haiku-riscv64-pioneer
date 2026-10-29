/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit native empty-commit readout TEST required
#endif
static void CheckNativeEmptyReadout(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;const unsigned before=io.touches,packets=io.runtimePackets;
 assert(!lockHeld && state->TerminalHeld() && !state->Closed() && !state->Unsafe());
 PioneerQ38TokenReadResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 std::array<uint32_t,4097> output={}; // valid capacity even for the largest refused64-record request
 assert(!ReadQ38TokenNativeRecords(c,0,nullptr,result) && io.touches==before);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!ReadQ38TokenNativeRecords(c,0,nullptr,result) && io.touches==before);result=pristine;
  }
  for(unsigned count:{1U,64U,65U,0xffffffffU})assert(!ReadQ38TokenNativeRecords(c,count,output.data(),result) && io.touches==before);
  assert(!ReadQ38TokenNativeRecords(c,0,output.data(),result) && !ReadQ38TokenNativeRecords(c,1,nullptr,result) && io.touches==before);
  for(void* alias:{static_cast<void*>(&c),static_cast<void*>(&io),static_cast<void*>(state),static_cast<void*>(&result)})
   assert(!ReadQ38TokenNativeRecords(c,1,static_cast<uint32_t*>(alias),result) && io.touches==before);
  auto* misaligned=reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(output.data())+1);
  assert(!ReadQ38TokenNativeRecords(c,1,misaligned,result) && io.touches==before);
  const bool okay=ReadQ38TokenNativeRecords(c,0,nullptr,result);
  if(!okay)fprintf(stderr,"EMPTY_READOUT_DIAGNOSTIC phase%u worker%u word%u packets%u unsafe%u;NO record supplied\n",
   result.phase,result.worker,result.word,result.packets,result.unsafe);
  assert(okay && result.started && result.ready && result.drained && !result.firstRecord && !result.recordCount
   && !result.packets && !result.bytesRead && !result.unsafe && state->TerminalHeld() && !state->Closed());
  const unsigned touches=io.touches;PioneerQ38TokenReadResult again;
  assert(!ReadQ38TokenNativeRecords(c,0,nullptr,again) && io.touches==touches);
 }
 for(auto value:output)assert(!value);
 assert(io.runtimePackets==packets && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased);
 puts("EXECUTED native SAME original layer0/cancel consumed/ROOT+119 real STOP+native terminal+120 real HOLD/actual ZERO-committed frontier drains WITHOUT GDDR packet or invented token/all malformed count+buffer+result refusals preserve output;NOT positive token-record/GDDR read/END/physical/Work/TPS qualification");
}
