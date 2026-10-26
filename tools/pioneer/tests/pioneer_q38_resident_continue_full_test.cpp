/* SPDX-License-Identifier: MIT */
// ACTUAL complete two-layer instruction path, SAME119 physical machines.
// Only raw learned/state/input bytes and NoC endpoints are synthetic.
// No ready/completed producer, expected numeric output or sequence reset.
#define PIONEER_Q38_CONTINUE_JOINED_MACHINE_ONLY
#include "pioneer_q38_resident_continue_joined_test.cpp"
struct ContinueFull:ContinueJoined {
 std::unique_ptr<PioneerQ38ResidentLayerPlan> following=std::make_unique<PioneerQ38ResidentLayerPlan>(),heldParent;
 bool second=false,secondVerified=false,finalFaults=false,finalForked=false;
 unsigned earlierPackets=0,reloadedSequences=0,secondReturn=0;
 std::array<uint32_t,2048> savedContinue={};
 ContinueFull(const char* image,const char* worker,const OutputParts& geometry):ContinueJoined(image,worker,geometry){
  assert(!originalBytes); // explicitly synthetic raw-byte test,not an original-source alias
  const auto* d=continuation->Descriptor();std::array<uint32_t,119> zero={};
  assert(following->Build(*canonical,d[3],d[4],d[6],d[10],d[5],d[9],{1,2,3,4},
   plan->Manifest()+TT_Q38_COHORT_SOURCE_WORD,zero.data()));
  const auto* next=continuation->Next();
  for(unsigned i=0;i<TT_Q38_RESIDENT_LAYER_WORDS;i++)assert(following->LayerManifest()[i]==next->LayerManifest()[i]);
  SeedLearned(*next); // RAW learned/current-history bytes BEFORE any execution
 }
 void SeedLearned(const PioneerQ38ResidentLayerPlan& next){
  const auto* m=next.Manifest();const auto* l=next.LayerManifest();
  for(unsigned i=0;i<5120;i++)Put(l[14],l[13]+i*4,Bits(.875f+float((i*23)%127)/512.f));
  for(unsigned w=0;w<120;w++){
   const auto* table=PioneerQ38CohortConv(m,w);
   for(unsigned j=0;j<PioneerQ38ConvBatchCount(w);j++){
    const auto* d=table+j*64;
    for(unsigned c=0;c<16;c++){
     for(unsigned t=0;t<4;t++)Put(d[13],d[12]+(c*4+t)*4,Bits(float(int(((d[7]+c)*7+t*11+13)%29)-14)*.0234375f));
     for(unsigned t=0;t<3;t++)Put(d[41],d[15]+(c*3+t)*4,Bits(float(cos((d[7]+c)*.013+t*.117+.419))));
    }
   }
   if(!w || w>96)continue;
   const auto* in=PioneerQ38CohortGdn(m,w);const auto* st=in+64;const auto* ga=st+64;
   for(unsigned i=0;i<16;i++){
    Put(in[13],in[12]+i*4,Bits(-float(i+1)*.1875f));Put(in[41],in[15]+i*4,Bits(.05f+float(i+1)*.0125f));
   }
   for(unsigned i=0;i<128;i++)Put(ga[13],ga[12]+i*4,Bits(float(.9+sin(i*.04)*.07)));
   for(unsigned i=0;i<8192;i++){
    Put(st[12],st[5]+i*4,Bits(float(sin((i+(w-1)*8192)*.003+.2))*.00017f));Put(st[13],st[7]+i*4,0x7fc00000);
   }
  }
  // In particular NO writes to projected A/B/Z/QKV, normalized input,
  // Conv/GDN products, ready/status/doorbells, code or tensor registers.
 }
 void EnterSecond(){
  assert(!second && !unknown && !pending && pc==secondEntry);VerifyAdmission();
  secondReturn=regs[1]; // ACTUAL return address of the following-layer call
  for(unsigned i=0;i<2048;i++)savedContinue[i]=Mem(TT_Q38_CONTINUE_RECEIPT+i*4);
  heldParent=std::move(plan);plan=std::move(following); // keep original parent lifetime intact
  for(unsigned i=0;i<manifest.size();i++)manifest[i]=Mem(TT_Q38_COHORT_MANIFEST+i*4);
  for(unsigned i=0;i<1152;i++)post[i]=Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+i*4);
  for(unsigned w=0;w<119;w++)assert(manifest[64+w]==initialSequence[w]);
  for(unsigned i=0;i<8;i++){initialNoc[i]=counts[i];assert(initialNoc[i]==Mem(TT_Q38_CONTINUE_RECEIPT+(1120+i)*4));}
  for(unsigned i=0;i<5120;i++)rawBefore[i]=Gddr(manifest[9],manifest[8]+98304+i*4);
  // Reset ONLY observer phase counts/coverage. ROOT physical counters,
  // registers, code, memory, tensors and every private worker sequence
  // remain untouched. The following actual entry captures its own data.
  earlierPackets=packets;packets=captures=postCaptures=layerCaptures=0;
  rootConvPackets=rootGroups=convLaunches=gdnLaunches=0;cleared.fill(false);verified.fill(false);
  outputLaunches=matrixLaunches=lineReads=compactWrites=nativeReads=scaleReads=resultWrites=0;
  gdnNumericsVerified=preparedVerified=false;coverage.fill(0);
  normPackets=ffnCaptures=ffnMatrices=ffnProducers=0;normSumStores=normStores=residualPackets=finalSumStores=0;
  prefixDone=normDone=ffnStarted=false;activationCoverage.fill(0);
  for(unsigned role=0;role<3;role++){assembled[role]=0;std::fill(expected[role].begin(),expected[role].end(),0x7fc00000);}
  for(unsigned w=0;w<119;w++){
   auto& p=NewPeer(w);const auto tensors=Tensor(p);std::array<uint32_t,32> regsBefore;
   for(unsigned i=0;i<32;i++)regsBefore[i]=p.regs[i];unsigned pcBefore=p.pc;
   assert(!p.active && !p.pending && p.pc==TT_Q38_PEER_WAIT_LOAD && p.memory.at(0x74144)==initialSequence[w]);
   uint32_t physical[8];for(unsigned i=0;i<8;i++)initialPeerNoc[w][i]=physical[i]=p.Count(i);
   // Carry completed response TRACE into the same independent physical
   // baseline BEFORE activation hides that trace. Not a hardware reset.
   p.NextPackets();for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];
   p.advance=p.ffn=p.outputActive=p.gdnActive=p.convActive=false;
   p.matrixJobs=p.producerJobs=0;p.ffnArithmetic=0;
   for(unsigned i=0;i<8;i++)assert(p.Count(i)==physical[i]);
   assert(p.pc==pcBefore && Tensor(p)==tensors);
   for(unsigned i=0;i<32;i++)assert(p.regs[i]==regsBefore[i]);
  }
  second=true;puts("ACTUAL_SECOND_LAYER_ENTER:admitted runtime recipe,ALL119 SAME machines/private sequences/physical counters/registers/tensors retained;observer bookkeeping ONLY,no device products or ready bits supplied");
 }
 void PrepareOutputBytes(CohortPeer& p){
  assert(second && !prefixDone && preparedVerified && !p.active && !p.requests && !p.responses && !p.matrixInstructions);
  const auto* part=post.data()+64+(p.logicalWorker-1)*8;
  p.type=8;p.inputs=6144;p.count=32;p.RowsMachine::groups=part[2];p.base=part[0];
  p.vector.resize(6144);p.scales.resize(192);p.root.clear();p.packets.clear();p.tile.assign(576,0);
  p.tiles.assign(part[2]*192,p.tile);
  for(unsigned t=0;t<p.tiles.size();t++)for(unsigned row=0;row<16;row++){
   unsigned global=post[1016+p.logicalWorker-1]+(t/192)*16+row;uint16_t half=uint16_t(0x0880+((global+1)%4)*0x80);
   p.tiles[t][row]=uint8_t(half);p.tiles[t][16+row]=uint8_t(half>>8);
   for(unsigned k=0;k<32;k++)p.tiles[t][(k+2)*16+row]=uint8_t(int((t*17+global*19+k*37+13)%255)-127);
  }
  p.inputReads=2;p.packets.push_back({0xa0000,129,0xa0000,12288,-1});p.packets.push_back({0xab000,129,0x94000,768,-1});
  for(unsigned t=0;t<p.tiles.size();t++)p.packets.push_back({part[0]+t*576,part[4],0x90000,576,int(t)});
  // Previous projections had MORE rows. Poison ONLY the output staging
  // observer before this new job,as the existing matrix/FFN fixtures do.
  // Never clear actual tensor Dst/registers,PC,sequence or status cells.
  for(unsigned i=0;i<4096;i++)p.buffers[4096+i]=0x7fc00000;
  // Existing LaunchMatrix decodes the ACTUAL device-prepared input wire
  // and scales; this creates RAW learned-byte endpoints, never products.
 }
 void Store(uint32_t at,uint32_t value)override{
  if(!second){ContinueJoined::Store(at,value);return;}
  if(Range(at,TT_Q38_CONTINUE_RECEIPT,8192)){assert(!pending && !unknown);Mem(at)=value;return;}
  if(at==0x17ffc8 && value==TT_Q38_CONTINUE_FAULT){unknown=true;Mem(at)=value;return;}
  if(Range(at,TT_Q38_COHORT_SEQUENCE,119*4) && !packets){
   unsigned w=(at-TT_Q38_COHORT_SEQUENCE)/4;
   assert(value==initialSequence[w] && value==NewPeer(w).memory.at(0x74144));reloadedSequences++;
  }
  LayerFabric::Store(at,value);
 }
 void Submit()override{
  if(!second){ContinueJoined::Submit();return;}
  if(!prefixDone && command[7]==0x212 && command[3]==0x74100){
   unsigned w=1;while(w<120 && PioneerQ38ConvPeer(w)!=command[5])w++;assert(w<120);auto& p=NewPeer(w-1);
   unsigned op=p.memory.at(0x74034);
   if(op==8)for(unsigned i=0;i<64;i++){
    p.input[i]=p.memory.at(TT_Q38_GDN_INPUT_DESCRIPTOR+i*4);p.state[i]=p.memory.at(TT_Q38_RGDN_DESCRIPTOR+i*4);p.gated[i]=p.memory.at(TT_Q38_GDN_GATED_DESCRIPTOR+i*4);
   }
   if(op==1)PrepareOutputBytes(p);
  }
  LayerFabric::Submit();
 }
 bool RunFull(){
  if(!RunAdmission())return false;EnterSecond();
  return RunSecond();
 }
 bool RunSecond(){
  while(pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f)){
   if(pc==secondReturn && !secondVerified){
    assert(second && regs[10]==5120 && !Mem(TT_Q38_CONTINUE_RECEIPT));VerifyLayer(false);secondVerified=true;
    puts("ACTUAL_SECOND_LAYER_RETURNED:all Conv/GDN/output/norm/357FFN jobs+88producers/final5120hidden and ALL119 private proof chains checked BEFORE wrapper publication");
    if(finalFaults)ForkFinalFailures();
   }
   StepRoot();
  }
  return !unknown;
 }
 void ForkFinalFailures(){
  assert(secondVerified && !finalForked && pc==secondReturn);finalForked=true;fflush(nullptr);
  for(unsigned kind=0;kind<9;kind++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    finalFaults=false;
    if(kind==0)Mem(TT_Q38_RESIDENT_LAYER_RECEIPT)^=1;
    if(kind==1)Mem(TT_Q38_RESIDENT_LAYER_RECEIPT+160*4)^=1;
    if(kind==2)Mem(TT_Q38_FFN_RESIDUAL_GRAPH_MANIFEST+64*4)^=1;
    if(kind==3)Mem(TT_Q38_FFN_RESIDUAL_GRAPH_STATS+24*4)^=1;
    if(kind==4)Mem(TT_Q38_CONTINUE_MANIFEST+3*4)^=1;
    if(kind==5)Mem(TT_Q38_RESIDENT_LAYER_SHADOW+3*4)^=1;
    if(kind==6)Mem(TT_Q38_COHORT_SHADOW+64*4)^=1;
    if(kind==7)counts[0]++;
    if(kind==8)source[11]^=1;
    unsigned before=packets;
    assert(!RunSecond() && unknown && !Mem(TT_Q38_CONTINUE_RECEIPT) && packets==before);
    Inert();assert(packets==before);
    printf("PASS ACTUAL_FULL_SECOND_RETURN_FORK_FAULT%u/completed REAL two-layer prefix/no QCP publication/no further ROOT packet/reset/retry;NOT native recovery\n",kind);
    fflush(nullptr);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 void VerifyFull(){
  assert(second && !unknown && !pending && reloadedSequences==119 && Mem(0x17ffc0)==donePc && Mem(0x17ffc4)==5120
   && Mem(0x17ffc8)==TT_Q38_CONTINUE_MARKER && Mem(TT_Q38_CONTINUE_RECEIPT)==TT_Q38_CONTINUE_RECEIPT_MARKER);
  assert(secondVerified);VerifyLayer(false);
  for(unsigned i=128;i<1128;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+i*4)==savedContinue[i]);
  for(unsigned i=0;i<128;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+i*4)==(i==0?unsigned(TT_Q38_CONTINUE_RECEIPT_MARKER):continuation->Descriptor()[i]));
  for(unsigned i=0;i<8;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(1128+i)*4)==counts[i]);
  for(unsigned i=0;i<10;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(1136+i)*4)==command[i]);
  for(unsigned i=1146;i<1152;i++)assert(!Mem(TT_Q38_CONTINUE_RECEIPT+i*4));
  for(unsigned w=0;w<119;w++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(1152+w)*4)==NewPeer(w).memory.at(0x74144)
   && NewPeer(w).memory.at(0x74144)==PioneerQ38ResidentLayerSequence(manifest.data(),w+1));
  for(unsigned i=1271;i<1280;i++)assert(!Mem(TT_Q38_CONTINUE_RECEIPT+i*4));
  for(unsigned i=0;i<256;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(1280+i)*4)==Mem(TT_Q38_RESIDENT_LAYER_RECEIPT+i*4));
  for(unsigned i=0;i<64;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+(1536+i)*4)==Mem(TT_Q38_COHORT_STATS+i*4)
   && Mem(TT_Q38_CONTINUE_RECEIPT+(1600+i)*4)==Mem(TT_Q38_FFN_RESIDUAL_GRAPH_STATS+i*4));
  for(unsigned i=1664;i<1668;i++)assert(Mem(TT_Q38_CONTINUE_RECEIPT+i*4)==savedContinue[i]);
  for(unsigned i=1668;i<2048;i++)assert(!Mem(TT_Q38_CONTINUE_RECEIPT+i*4));
  assert(minStack>=0x6e000);
 }
};
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--faults")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("BEGIN ACTUAL COMPLETE two recurrent layers with ONCARD link/projections/admission/SAME119 machines;synthetic raw learned/state bytes only,NO CPU numeric feedback or native/token-rate grant");
 OutputParts geometry(0,0);ContinueFull f(argv[1],argv[2],geometry);f.finalFaults=argc==4;
 if(!f.RunFull()){f.Diagnostic();fprintf(stderr,"FULL_SECOND_FAILURE second%u pc%08x ra%08x packets%u Conv%u/GDN%u/output%u/matrix%u norm%u FFN%u/%u\n",f.second,f.pc,f.regs[1],f.packets,f.convLaunches,f.gdnLaunches,f.outputLaunches,f.matrixLaunches,f.normPackets,f.ffnMatrices,f.ffnProducers);return 1;}
 f.VerifyFull();f.Inert();
 printf("PASS ACTUAL COMPLETE two-layer chain/ALL119retained sequences+physical BEFORE/AFTER chains/both Conv640/GDN96/output119/357FFNmatrices+88producers/final5120hidden checked/maxStack%u;NO numerical host feedback/reset/relaunch. SYNTHETIC endpoints,NOT original-model/native/silicon/full64layers/token loop or inference-speed qualification\n",0x70000-f.minStack);
}
