/* SPDX-License-Identifier: MIT */
// ACTUAL ROOT and ALL119 emitted peer programs; shared byte endpoints only.
// No numeric products, success receipts or host intermediate are injected.
#define PIONEER_Q38_RESIDENT_GDN_OUTPUT_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_gdn_output_operator_service_image_test.cpp"
#define PIONEER_Q38_COHORT_GDN_PEER_ALREADY_INCLUDED
#define PIONEER_Q38_COHORT_PEER_BASE ResidentGdnOutputPeer
#define PIONEER_Q38_COHORT_OUTPUT_PEER
#define PIONEER_Q38_RESIDENT_COHORT_MACHINE_ONLY
#include "pioneer_q38_resident_cohort_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputCohortShape.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "pioneer_q38_resident_output_learned_bytes.h"
struct OutputParts {
 std::unique_ptr<PioneerQ38CanonicalModelPlan> canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();
 std::unique_ptr<PioneerQ38ProjectionJobs> jobs=std::make_unique<PioneerQ38ProjectionJobs>();
 std::array<unsigned,119> groups={};unsigned layer,position;
 OutputParts(unsigned l,unsigned p):layer(l),position(p){
  assert(canonical->Build() && jobs->Build(*canonical,l,13,p) && jobs->Count()==119 && jobs->Waves()==1 && jobs->Inputs()==6144 && jobs->Outputs()==5120);
  for(unsigned w=0;w<119;w++)groups[w]=jobs->Part(w)->groups;
 }
};
struct OutputCohortFabric:CohortFabric {
 ProjectionExecutionLearnedBytes* originalBytes=nullptr; // TEST raw authenticated bytes ONLY
 std::vector<uint8_t> matrixCode;
 std::array<uint32_t,1152> post={};
 std::array<std::vector<uint8_t>,24> tensorBefore;
 std::array<uint32_t,6144> actualGated={};
 std::array<unsigned,5120> coverage={};
 unsigned outputLaunches=0,matrixLaunches=0,lineReads=0,compactWrites=0,nativeReads=0,scaleReads=0,resultWrites=0,postCaptures=0;
 bool gdnNumericsVerified=false,preparedVerified=false;
 unsigned mutateLine=0,mutateLineWord=0,badOutputProof=0,badOutputProofWord=0,badMatrixProof=0,badMatrixProofWord=0;
 unsigned dropOutputWorker=0,dropMatrixWorker=0,postMutationPacket=0;
 unsigned pairedCounter=0,latePostLine=0,lateRootSourceLine=0;
 bool foreignGatherCommand=false;unsigned gatherCommandReads=0;
 static std::vector<uint8_t> Tensor(const CohortPeer& p){
  std::vector<uint8_t> result;
  auto append=[&](const auto& value){const uint8_t* b=reinterpret_cast<const uint8_t*>(&value);result.insert(result.end(),b,b+sizeof(value));};
  append(p.config);append(p.thread);append(p.lr);append(p.physical);append(p.dstInt);append(p.defined);append(p.wrote);
  return result;
 }
 OutputCohortFabric(const char* root,const char* worker,const OutputParts& geometry,unsigned bank=0,unsigned slot=0)
  :CohortFabric(root,worker,geometry.layer,bank,geometry.position,slot,geometry.groups.data()),matrixCode(Read((std::string(root)+"/matrix.bin").c_str())){
  assert(!matrixCode.empty() && !(matrixCode.size()&3) && matrixCode.size()<=0x6000);
  auto& q=post;const auto& m=manifest;
  q[0]=TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER;q[1]=1;q[2]=1152;q[3]=m[3];q[4]=m[4];q[5]=m[6]+1;q[6]=q[5]+1;q[7]=q[6]+1;
  q[8]=m[7];q[9]=m[8];q[10]=m[9];q[11]=248319;q[12]=6144;q[13]=5120;q[14]=119;
  for(unsigned i=16;i<40;i++)q[i]=m[i];
  for(unsigned w=0;w<119;w++){
   const auto& part=*geometry.jobs->Part(w);const uint32_t p[]={0x04000000+uint32_t(part.offset),part.bank,part.groups,0,PioneerQ38ProjectionRoute(part.bank),peers[w]->source[0],0,0};
   for(unsigned i=0;i<8;i++)q[64+w*8+i]=p[i];q[1016+w]=part.firstRow;
  }
  for(unsigned i=0;i<119*64;i++)Mem(0x76000+i*4)=0x7fc00000;
  assert(PioneerQ38ResidentOutputCohortShape(q.data(),m.data()));
  for(unsigned i=0;i<1152;i++)Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+i*4)=q[i];
  // Output offset100000 temporally reuses raw A. Do NOT poison that future
  // destination before actual GDN consumes it. Exact fresh-write coverage
  // and independent ALL5120-row checks below prove final publication.
 }
 bool CodeAddress(uint32_t at)const override{return Range(at,0x68000,unsigned(matrixCode.size())) || CohortFabric::CodeAddress(at);}
 unsigned Index(uint32_t at)override{
  if(Range(at,0x68000,unsigned(matrixCode.size()))){assert(!(at&3));l1[8179]=Word(matrixCode,at-0x68000);return 8179;}
  return CohortFabric::Index(at);
 }
 bool RangeData(unsigned at)const override{return CohortFabric::RangeData(at)
  || Range(at,TT_Q38_OUTPUT_COHORT_MANIFEST,1152*8) || Range(at,0x76000,0xa000)
  || Range(at,0x80000,20480) || Range(at,0x90e00,128) || Range(at,0xa0000,12288) || Range(at,0xab000,768);}
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x68000,unsigned(matrixCode.size())))return Word(matrixCode,at-0x68000);
  if(foreignGatherCommand && lineReads==1 && Range(at,0xffb20000,40)){
   uint32_t value=CohortFabric::Load(at);if(++gatherCommandReads==10)command[1]^=1;return value;
  }
  return CohortFabric::Load(at);
 }
 uint32_t ReadCycleCsr(unsigned csr)override{assert(csr==0xb00 || csr==0xb80);return csr==0xb00?uint32_t(clock):uint32_t(clock>>32);}
 void Store(uint32_t at,uint32_t value)override{
  assert(!Range(at,TT_Q38_OUTPUT_COHORT_MANIFEST,1152*4));
  if(Range(at,TT_Q38_OUTPUT_COHORT_SHADOW,1152*4)){
   // Capture the ACTUAL supplied bytes, including deliberately malformed
   // pre-I/O test inputs. The emitted shape check must reject those after
   // its faithful snapshot; this fixture must not reject them prematurely.
   assert(!packets && postCaptures<1152 && at==TT_Q38_OUTPUT_COHORT_SHADOW+postCaptures*4 && value==Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+postCaptures*4));postCaptures++;
  }
  if(at==0x17ffc8 && value==TT_Q38_RESIDENT_OUTPUT_COHORT_FAULT)unknown=true;
  CohortFabric::Store(at,value);
 }
 void Retire(unsigned bytes,bool write){unsigned base=write?4:0;counts[base]++;counts[base+1]+=(bytes+63)/64;counts[base+3]++;counts[2]++;counts[6]++;}
 void VerifyGdnStage(){
  assert(!gdnNumericsVerified && convLaunches==119 && gdnLaunches==96 && !outputLaunches);
  VerifyHistoryAndGdn();gdnNumericsVerified=true;
  for(unsigned i=0;i<6144;i++)actualGated[i]=Gddr(manifest[9],manifest[8]+TT_Q38_GDN_GATED_OUTPUT+i*4);
  for(unsigned w=0;w<24;w++)tensorBefore[w]=Tensor(*peers[w]);
 }
 void VerifyPrepared(){
  assert(gdnNumericsVerified && !preparedVerified && outputLaunches==24);
  for(unsigned w=0;w<24;w++){
   auto& p=*peers[w];assert(!p.active && p.GdnFinished(manifest[64+w]+3));p.VerifyOutput();assert(Tensor(p)==tensorBefore[w]);
   for(unsigned i=0;i<128;i++)assert(Gddr(manifest[9],manifest[8]+TT_Q38_PROJECTION_NATIVE_OFFSET+w*512+i*4)==p.native[i]);
   for(unsigned i=0;i<32;i++)assert(Gddr(manifest[9],manifest[8]+TT_Q38_PROJECTION_LINES_OFFSET+w*128+i*4)==p.line[i]);
  }
  preparedVerified=true;
 }
 void LaunchOutput(CohortPeer& p,unsigned seq){
  assert(gdnNumericsVerified && !p.active && seq==p.launched+1 && p.memory.at(0x74030)==seq && p.logicalWorker<=24);
  p.ClearTensorState();p.launched=seq;p.gdnActive=p.convActive=false;p.outputActive=p.activation=true;
  p.activationPackets=p.commandReads=p.activatedWrites=p.nativeWrites=p.scaleWrites=p.lineWrites=p.receiptReads=p.packetCommandReads=0;
  for(unsigned i=0;i<64;i++)p.descriptor[i]=p.memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);
  for(unsigned i=0;i<8;i++)p.independentBefore[i]=p.Count(i);
  p.active=true;if(p.logicalWorker==dropOutputWorker)p.failPacket=1;
 }
 void LaunchMatrix(CohortPeer& p,unsigned seq){
  assert(preparedVerified && !p.active && seq==p.launched+1 && p.memory.at(0x74030)==seq && !p.requests && !p.responses && !p.matrixInstructions);
  p.ClearTensorState();p.outputActive=p.gdnActive=p.convActive=p.activation=false;p.launched=seq;
  if(!matrixLaunches){
   for(unsigned i=0;i<3072;i++)assert(Mem(0xa0000+i*4)==Gddr(manifest[9],manifest[8]+TT_Q38_PROJECTION_NATIVE_OFFSET+i*4));
   for(unsigned i=0;i<192;i++)assert(Mem(0xab000+i*4)==peers[i/8]->line[i%8]);
  }
  // Endpoints and the independent numeric observer consume the ACTUAL root
  // fetched native wire. Nothing computes or feeds a matrix result here.
  for(unsigned i=0;i<3072;i++)p.root[0xa0000+i*4]=Mem(0xa0000+i*4);
  for(unsigned i=0;i<192;i++){p.root[0xab000+i*4]=Mem(0xab000+i*4);p.scales[i]=Mem(0xab000+i*4);}
  for(unsigned i=0;i<6144;i++)p.vector[i]=uint16_t(p.root.at(0xa0000+i/2*4)>>((i%2)*16));
  for(unsigned i=p.inputReads;i<p.packets.size();i++){
   p.packets[i].address=p.memory.at(0x74018)+(i-p.inputReads)*576;p.packets[i].route=p.memory.at(0x74020);
  }
  if(originalBytes){
   const auto* part=post.data()+64+(p.logicalWorker-1)*8;
   assert(part[0]==p.memory.at(0x74018) && part[4]==p.memory.at(0x74020) && part[2]==p.RowsMachine::groups
    && ResidentOutputImportMatrixBytes(part,*originalBytes,p.tiles));
  } // A present original source can NEVER silently fall back to synthetic tiles.
  p.Poison();p.PoisonTail();p.active=true;if(p.logicalWorker==dropMatrixWorker)p.faultAt=1;
 }
 void Submit()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],remote=write?command[3]:command[0],local=write?command[0]:command[3],bytes=command[8];
  if(Range(pc,0x64000,unsigned(sections[2].size()))){CohortFabric::Submit();return;}
  if(route==manifest[9]){
   packets++;if(packets==failPacket){pending=true;return;}
   assert(!command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129);
   if(!write && local==0x90e00){
    if(!preparedVerified)VerifyPrepared();assert(bytes==128 && remote==manifest[8]+TT_Q38_PROJECTION_LINES_OFFSET+lineReads*128);
    lineReads++;
   }else if(!write && local==0xa0000){assert(preparedVerified && lineReads==24 && compactWrites==1 && bytes==12288 && remote==manifest[8]+TT_Q38_PROJECTION_NATIVE_OFFSET);nativeReads++;
   }else if(local==0xab000){assert(preparedVerified && lineReads==24 && bytes==768 && remote==manifest[8]+TT_Q38_PROJECTION_COMPACT_OFFSET);if(write)compactWrites++;else scaleReads++;
   }else {
    assert(write && matrixLaunches==119 && local>=0x80000 && local+bytes<=0x85000 && remote>=manifest[8]+TT_Q38_PROJECTION_OUTPUT_OFFSET);
    unsigned first=(remote-manifest[8]-TT_Q38_PROJECTION_OUTPUT_OFFSET)/4;assert(first<5120 && bytes/4<=5120-first);
    for(unsigned i=0;i<bytes/4;i++)assert(!coverage[first+i]++);resultWrites++;
   }
   for(unsigned i=0;i<bytes/4;i++){
    if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4)^((lineReads==mutateLine && local==0x90e00 && i==mutateLineWord)?1U:0U);
   }
   Retire(bytes,write);
   if(packets==postMutationPacket || (latePostLine && !write && local==0x90e00 && lineReads==latePostLine))Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+11*4)^=1;
   if(lateRootSourceLine && !write && local==0x90e00 && lineReads==lateRootSourceLine)source[11]^=1;
   return;
  }
  unsigned w=1;while(w<120 && PioneerQ38ConvPeer(w)!=route)w++;assert(w<120);auto& p=*peers[w-1];
  // Observe completed GDN BEFORE the first actual ROOT write reuses its
  // descriptor/job mailboxes for op6. This observer never feeds a result.
  if(write && remote==TT_Q38_FFN_RESIDENT_DESCRIPTOR && bytes==256 && Mem(local)==TT_Q38_RESIDENT_GDN_OUTPUT_MARKER && !gdnNumericsVerified)VerifyGdnStage();
  if(write && remote==0x74100 && (p.memory.at(0x74034)==6 || p.memory.at(0x74034)==1)){
   packets++;if(packets==failPacket){pending=true;return;}
   assert(bytes==4 && !p.active);
   if(p.memory.at(0x74034)==6){assert(gdnNumericsVerified);LaunchOutput(p,Mem(local));outputLaunches++;}
   else {LaunchMatrix(p,Mem(local));matrixLaunches++;}
   p.memory[0x74100]=Mem(local);Retire(bytes,true);return;
  }
  if(!write && remote==0x83f00 && bytes>256){
   packets++;if(packets==failPacket){pending=true;return;}
   assert(p.GdnFinished(p.launched) && !p.active && bytes==256+p.RowsMachine::groups*64);
   for(unsigned i=0;i<bytes/4;i++){
    uint32_t value=i<64?p.memory.at(remote+i*4):p.buffers[4096+i-64];
    if(w==1 && pairedCounter && (i==pairedCounter-1 || i==pairedCounter+7))value++;
    if(w==1 && pairedCounter==3 && (i==6 || i==14))value++;
    if(w==badMatrixProof && i==badMatrixProofWord)value^=1;Mem(local+i*4)=value;
   }
   Retire(bytes,false);return;
  }
  if(!write && remote==0x83f00 && bytes==256 && p.memory.at(0x74034)==6 && w==badOutputProof){
   unsigned saved=badProofWorker,word=badProofWord,op=badProofOperation;
   badProofWorker=w;badProofWord=badOutputProofWord;badProofOperation=6;CohortFabric::Submit();badProofWorker=saved;badProofWord=word;badProofOperation=op;return;
  }
  CohortFabric::Submit();
 }
 void Verify(bool stopped=false){
  assert(!pending && !unknown && postCaptures==1152 && Mem(0x17ffc0)==donePc && Mem(0x17ffc4)==5120 && Mem(0x17ffc8)==TT_Q38_RESIDENT_OUTPUT_COHORT_MARKER);
  assert(gdnNumericsVerified && preparedVerified && convLaunches==119 && gdnLaunches==96 && outputLaunches==24 && matrixLaunches==119);
  assert(lineReads==24 && compactWrites==1 && nativeReads==1 && scaleReads==1 && resultWrites==119);
  assert(Mem(TT_Q38_COHORT_STATS+7*4)==24 && Mem(TT_Q38_COHORT_STATS+8*4)==24 && Mem(TT_Q38_COHORT_STATS+9*4)==5120);
  for(unsigned w=0;w<119;w++){
   auto& p=*peers[w];unsigned seq=manifest[64+w]+(w<24?4:w<96?3:2);
   // The public control returns AFTER genuine STOP,not at RUN's WAIT PC.
   // Verify the ACTUAL newer terminal/nonce/receipt in that case;never
   // replace either lifecycle proof with a cached ready/completion flag.
   if(stopped)assert(p.pc==TT_Q38_PEER_STOP && !p.pending
    && p.memory.at(0x74140)==seq+1 && p.memory.at(0x74144)==seq+1
    && p.memory.at(0x7414c)==TT_Q38_FFN_PERSISTENT_MARKER && !p.memory.at(0x74148) && p.memory.at(0x74150)==2);
   else assert(p.GdnFinished(seq));
   assert(!p.active && p.CorrectRows() && !p.dotProbeReads && Mem(TT_Q38_COHORT_SEQUENCE+w*4)==seq);
   unsigned first=post[1016+w];
   for(unsigned i=0;i<p.RowsMachine::groups*16;i++)assert(coverage[first+i]==1 && Gddr(manifest[9],manifest[8]+TT_Q38_PROJECTION_OUTPUT_OFFSET+(first+i)*4)==p.buffers[4096+i]);
  }
 }
};
void CohortPeer::SubmitActivation(){
 assert(outputActive && fabric);unsigned before=activationPackets;
 if(!before)for(unsigned i=0;i<256;i++)gate[i]=fabric->Gddr(descriptor[8],descriptor[4]+descriptor[10]+i*4);
 ResidentGdnOutputPeer::SubmitActivation();
 if(!pending && before==1)for(unsigned i=0;i<128;i++)fabric->Put(descriptor[8],descriptor[4]+descriptor[12]+i*4,native[i]);
 if(!pending && before==2)for(unsigned i=0;i<32;i++)fabric->Put(descriptor[8],descriptor[4]+descriptor[13]+i*4,line[i]);
}
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_COHORT_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--smoke") || !strcmp(argv[3],"--positives") || !strcmp(argv[3],"--faults") || !strcmp(argv[3],"--terminal-faults"))));setvbuf(stdout,nullptr,_IOLBF,0);
 bool smoke=argc==4 && !strcmp(argv[3],"--smoke"),terminalOnly=argc==4 && !strcmp(argv[3],"--terminal-faults");
 bool faultsOnly=terminalOnly || (argc==4 && !strcmp(argv[3],"--faults"));(void)&RunPersistentGdnPair;
 for(unsigned variant=0;variant<(smoke?1U:3U);variant++){
  if(faultsOnly)break;
  OutputParts parts(variant==0?0:variant==1?62:30,variant==0?0:variant==1?8191:5);
  OutputCohortFabric f(argv[1],argv[2],parts,variant==0?0:variant==1?6:3,variant&1);
  bool okay=f.Run();
  if(!okay){
   fprintf(stderr,"OUTPUT_COHORT_FAULT pc%08x ra%08x steps%u packets%u conv%u gdn%u output%u matrix%u lines%u native%u scale%u result%u lastRemote%08x bytes%u checkpoint%08x/%08x\n",f.pc,f.regs[1],f.steps,f.packets,f.convLaunches,f.gdnLaunches,f.outputLaunches,f.matrixLaunches,f.lineReads,f.nativeReads,f.scaleReads,f.resultWrites,f.command[7]==0x212?f.command[3]:f.command[0],f.command[8],f.Mem(0x17ffc0),f.Mem(0x17ffc4));
   for(unsigned w=0;w<119;w++)if(f.peers[w]->GdnFaulted())fprintf(stderr,"PEER_FAULT worker%u ra%08x op%u seq%u packets%u/%u/%u\n",w+1,f.peers[w]->regs[1],f.peers[w]->memory.at(0x74034),f.peers[w]->launched,f.peers[w]->convPackets,f.peers[w]->gdnPackets,f.peers[w]->activationPackets);
   if(f.peers[0]->GdnFaulted()){
    auto& p=*f.peers[0];fprintf(stderr,"PEER1 command");for(unsigned v:p.command)fprintf(stderr," %08x",v);fprintf(stderr,"\nPEER1 counters");for(unsigned i=0;i<8;i++)fprintf(stderr," %08x",p.Count(i));fprintf(stderr,"\nPEER1 payload");for(unsigned i=0;i<16;i++)fprintf(stderr," %08x",p.memory.at(0x80800+i*4));fprintf(stderr,"\nPEER1 inputSummary%u frame%08x tracked%08x\n",p.memory.at(TT_Q38_GDN_INPUT_SUMMARY),p.regs[8],p.regs[22]);
   }
  }
  assert(okay);f.Verify();f.Inert();
  printf("PASS ACTUAL ROOT+ALL119 peers/ALL640Conv->96GDN->24QOL1->fullK6144/N5120 matrix/ALL5120 independent FP32+FP64 rows/same physical state/NO host intermediate;variant%u steps%u packets%u maxStack%u. SYNTHETIC weight endpoints,NOT native/token-loop/silicon speed\n",variant,f.steps,f.packets,0x70000-f.minStack);
 }
 if(argc==4 && !faultsOnly){puts("PASS PARTIAL combined pipeline positives ONLY;fault suite intentionally skipped");return 0;}
 unsigned faults=0;OutputParts parts(0,0);
 if(!terminalOnly){
 for(unsigned word=0;word<64;word++){
  OutputCohortFabric f(argv[1],argv[2],parts);f.badOutputProof=1;f.badOutputProofWord=word;
  assert(!f.Run() && f.outputLaunches==24 && !f.matrixLaunches && !f.lineReads);f.Inert();faults++;
  printf("PASS actual ROOT rejects QOL1 proof word%u before gather/matrix\n",word);
 }
 for(unsigned word:{0U,8U,16U,17U,18U,19U,20U,27U,30U,31U,32U,48U,63U}){
  OutputCohortFabric f(argv[1],argv[2],parts);f.badMatrixProof=1;f.badMatrixProofWord=word;
  assert(!f.Run() && f.matrixLaunches==119 && !f.resultWrites);f.Inert();faults++;
  printf("PASS actual ROOT rejects matrix proof word%u before scatter\n",word);
 }
 for(unsigned counter:{1U,2U,3U,4U,5U,6U,8U}){
  OutputCohortFabric f(argv[1],argv[2],parts);f.pairedCounter=counter;
  assert(!f.Run() && f.matrixLaunches==119 && f.resultWrites==119 && f.Mem(0x7f800+56*4)==1 && f.Mem(0x17ffc8)==TT_Q38_RESIDENT_OUTPUT_COHORT_FAULT);
  f.Inert();faults++;printf("PASS paired before/after counter%u preserves matrix delta but new actual-prefix chain rejects final success\n",counter-1);
 }
 for(unsigned word=8;word<32;word++){
  OutputCohortFabric f(argv[1],argv[2],parts);f.mutateLine=1;f.mutateLineWord=word;
  assert(!f.Run() && f.lineReads==1 && !f.compactWrites && !f.matrixLaunches);f.Inert();faults++;
 }
 puts("PASS ALL24 fresh line ancestry word corruptions reject before compact publication/native read/matrix");
 }
 for(unsigned kind=0;kind<3;kind++){
  OutputCohortFabric f(argv[1],argv[2],parts);
  if(!kind)f.latePostLine=1;if(kind==1)f.lateRootSourceLine=1;if(kind==2)f.foreignGatherCommand=true;
  assert(!f.Run() && f.lineReads==1 && !f.compactWrites && !f.matrixLaunches);f.Inert();faults++;
 }
 for(unsigned kind=0;kind<2;kind++){
  OutputCohortFabric f(argv[1],argv[2],parts);if(!kind)f.dropOutputWorker=1;else f.dropMatrixWorker=1;
  assert(!f.Run() && (!kind?!f.lineReads:!f.resultWrites));f.Inert();faults++;
 }
 for(unsigned at:{0U,5U,64U,1016U}){
  OutputCohortFabric f(argv[1],argv[2],parts);f.Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+at*4)^=1;
  assert(!f.Run() && !f.packets && !f.convLaunches && !f.gdnLaunches);f.Inert();faults++;
 }
 if(terminalOnly)printf("PASS PARTIAL terminal-only combined coordinator faults%u/late source+post+command/lost transfers/faithful snapshot of malformed inputs rejected by ACTUAL ROOT before ANY NoC packet;earlier receipt tests intentionally NOT rerun/NO native grant\n",faults);
 else printf("PASS actual combined coordinator faults%u/ALL64QOL proof mutations/matrix receipts+nontrivial prefix-counter chain/fresh line ancestry/late source+post+command/lost transfer/pre-I/O malformed manifests;NO native/token-loop/physical speed grant\n",faults);
}
#endif
