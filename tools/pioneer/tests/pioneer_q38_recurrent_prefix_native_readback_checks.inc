/* SPDX-License-Identifier: MIT */
// Inside the actual post-RUN native fixture,not a fabricated ready owner.
 std::vector<uint32_t> readbackValues(5120,0xdeadbeef);PioneerQ38RecurrentPrefixReadbackResult readback;
 unsigned inertReadbacks=0;
 auto inertReadback=[&](uint32_t* p,unsigned words){
  auto physical=transport.Touches(),touches=io.touches;
  assert(!CloseQ38RecurrentPrefixNativeValidation(c,execution,memory,p,words,readback) && !state->Unsafe()
   && !readback.started && touches==io.touches && physical==transport.Touches());inertReadbacks++;
 };
 inertReadback(nullptr,5120);inertReadback(readbackValues.data(),5119);
 for(void* object:{static_cast<void*>(&c),static_cast<void*>(state),static_cast<void*>(c.q38EmbeddingModel),
  static_cast<void*>(&execution),static_cast<void*>(&memory),static_cast<void*>(&readback),static_cast<void*>(c.q38Dispatch)})
  inertReadback(reinterpret_cast<uint32_t*>(object),5120);
 assert(inertReadbacks==9);
 for(unsigned fault=1;fault<=3;fault++)RecurrentNativeFork([&]{
  transport.EnableValidationReadback(fault);
  assert(!CloseQ38RecurrentPrefixNativeValidation(c,execution,memory,readbackValues.data(),5120,readback)
   && transport.ReadbackFaultFence() && transport.Touches()==transport.ReadbackFaultFence()
   && state->Unsafe() && c.dispatchOwnerRetained && c.banks.Quarantined() && c.q38Clock.quarantined
   && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired
   && execution.held==120 && !memory.restoredWords && !readback.completed);
  for(auto value:readbackValues)assert(value==0xdeadbeef);
  auto physical=transport.Touches(),touches=io.touches,resets=transport.Resets();
  assert(!CloseQ38RecurrentPrefixNativeValidation(c,execution,memory,readbackValues.data(),5120,readback)
   && !CloseQ38RecurrentPrefixNativeExecution(c,execution,memory) && !EndFactory(c)
   && physical==transport.Touches() && touches==io.touches && resets==transport.Resets());
  printf("PASS QPX actual native readback fault%u/real3RUN+STOP+120holds/ALL owners retained/ZERO following IO,restore,retry or caller output\n",fault);
 });
 transport.EnableValidationReadback();
 bool closed=CloseQ38RecurrentPrefixNativeValidation(c,execution,memory,readbackValues.data(),5120,readback);
 if(closed){
  assert(readback.started && readback.completed && readback.pages==5 && readback.words==5120 && transport.ReadbackPackets()==5);
  transport.VerifyReadback(readbackValues.data(),5120);
  puts("PASS QPX genuine native3RUN/STOP+ALL120holds/fivefixed LAST-layer2 READs/capture5120/full restore before publication/9inert requests+3reached readback faults;raw endpoints SIMULATED,NOT original-model physical or Work/TPS proof");
 }
