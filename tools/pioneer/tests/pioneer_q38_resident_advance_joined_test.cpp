/* SPDX-License-Identifier: MIT */
// Actual full layer/link/next input and all four projection matrices in ONE
// ROOT call. Same119 machines, tensor/register/private sequence history.
// Simulated byte endpoints/raw learned bytes only; NO numeric feedback.
#define PIONEER_Q38_LAYER_LINK_JOINED_MACHINE_ONLY
#include "pioneer_q38_resident_layer_link_joined_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentAdvancePlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionMatrixShape.h"
#include <sys/wait.h>
#include <unistd.h>
struct AdvanceFabric;
struct AdvancePeer:LayerPeer {
 AdvanceFabric* advanceFabric=nullptr;bool advance=false;unsigned advanceRole=0,advanceSequence=0;
 explicit AdvancePeer(LayerPeer&& p):LayerPeer(std::move(p)){}
 void PrepareAdvance();void SubmitActivation()override;void StepBlock()override;void VerifyProjection();
};
struct AdvanceFabric:LinkedLayerFabric {
 PioneerQ38ResidentAdvancePlan advancePlan;
 std::array<std::vector<uint8_t>,3> advanceCode;
 unsigned advanceEntry=0,advanceCaptures=0,advanceJobs=0,advanceProducers=0,gatherPackets=0,matrixInputPackets=0,matrixOutputPackets=0;
 bool advancing=false,forkFaults=false,forked=false;unsigned activeRole=0;
 unsigned advancePackets=0,failAdvancePacket=0,counterFault=0,rootSourceFault=0,manifestFault=0;
 std::array<uint32_t,16> recentPc={};unsigned recentAt=0;
 std::array<std::vector<uint32_t>,4> observed;
 std::array<std::vector<unsigned>,4> cover;
 std::array<unsigned,4> proofStores={};
 AdvanceFabric(const char* image,const char* peer,const OutputParts& geometry):LinkedLayerFabric(image,peer,geometry){
  assert(advancePlan.Build(*canonical,*plan,link));
  for(unsigned i=0;i<TT_Q38_ADVANCE_WORDS;i++)Mem(TT_Q38_ADVANCE_MANIFEST+i*4)=advancePlan.Descriptor()[i];
  for(unsigned i=0;i<3;i++)advanceCode[i]=Read((std::string(image)+"/"+(i==0?"advance_matrix":i==1?"advance":"advance_gather")+".bin").c_str());
  advanceEntry=Symbol(image,"q38_resident_advance_once");
  for(unsigned i=0;i<4;i++){unsigned rows=PioneerQ38ProjectionResultRows(PioneerQ38ResidentAdvanceRole(i));observed[i].assign(rows,0x7fc00001);cover[i].assign(rows,0);}
  for(unsigned w=0;w<119;w++){
   auto p=std::make_unique<AdvancePeer>(std::move(Peer(w)));p->advanceFabric=this;peers[w]=std::move(p);
  }
  for(unsigned w=0;w<96;w++)peers[w]->peer=peers[w^1].get();
 }
 AdvancePeer& NewPeer(unsigned w){return static_cast<AdvancePeer&>(*peers.at(w));}
 unsigned NewSection(unsigned at)const{return Range(at,0x40000,unsigned(advanceCode[0].size()))?0:Range(at,0x46000,unsigned(advanceCode[1].size()))?1:Range(at,0x4e000,unsigned(advanceCode[2].size()))?2:3;}
 unsigned NewBase(unsigned i)const{return i==0?0x40000:i==1?0x46000:0x4e000;}
 uint32_t NewWord(unsigned at)const{unsigned i=NewSection(at);assert(i<3);return Word(advanceCode[i],at-NewBase(i));}
 bool CodeAddress(uint32_t at)const override{return NewSection(at)<3 || LinkedLayerFabric::CodeAddress(at);}
 unsigned Index(uint32_t at)override{if(NewSection(at)<3){l1[8176]=NewWord(at);return 8176;}return LinkedLayerFabric::Index(at);}
 bool RangeData(unsigned at)const override{return LinkedLayerFabric::RangeData(at) || Range(at,0x50000,0xb000);}
 uint32_t Load(uint32_t at)override{return NewSection(at)<3?NewWord(at):LinkedLayerFabric::Load(at);}
 void Store(uint32_t at,uint32_t v)override{
  if(Range(at,TT_Q38_ADVANCE_SHADOW,TT_Q38_ADVANCE_WORDS*4)){
   assert(!packets && advanceCaptures<TT_Q38_ADVANCE_WORDS && at==TT_Q38_ADVANCE_SHADOW+advanceCaptures*4
    && v==Mem(TT_Q38_ADVANCE_MANIFEST+advanceCaptures*4));advanceCaptures++;Mem(at)=v;return;
  }
  if(Range(at,TT_Q38_ADVANCE_RECEIPT,TT_Q38_ADVANCE_RECEIPT_WORDS*4)){assert(!pending && !unknown);Mem(at)=v;return;}
  if(at==0x17ffc8 && v==TT_Q38_ADVANCE_FAULT){unknown=true;Mem(at)=v;return;}
  if(advancing && at==TT_Q38_NEXT_PROJECTION_MATRIX_FRAME+43*4){
   activeRole=PioneerQ38ProjectionResultSlot(v);assert(activeRole<4);
  }
  if(advancing && Range(at,0x76000,119*256)){
   // The completed layer's temporary FFN manifest/shadow occupied this
   // same scratch span. VerifyLayer(false) already inspected them before
   // EnterLink. The persistent layer plan/receipt stay elsewhere, intact.
   // Permit ONLY the new matrix's sequential copy of its actual current
   // peer proof, not arbitrary stores or a fabricated completion receipt.
   const auto* r=PioneerQ38ResidentAdvanceRecord(advancePlan.Descriptor(),activeRole);
   unsigned& copied=proofStores[activeRole];
   assert(!pending && !unknown && Range(pc,0x40000,unsigned(advanceCode[0].size()))
    && copied<r[2]*64 && at==0x76000+copied*4 && v==Mem(0x75000+(copied%64)*4));
   copied++;Mem(at)=v;return;
  }
  LinkedLayerFabric::Store(at,v);
 }
 void Submit()override{
  if(!advancing){LinkedLayerFabric::Submit();return;}
  packets++;advancePackets++;
  if(advancePackets==failAdvancePacket){pending=true;return;}
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(!command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  if(route==manifest[9]){
   assert(remote>=manifest[8] && bytes && bytes<=16384 && !(bytes&63));
   if(Range(pc,0x4e000,unsigned(advanceCode[2].size())))gatherPackets++;
   else {assert(Range(pc,0x40000,unsigned(advanceCode[0].size())));if(write)matrixOutputPackets++;else matrixInputPackets++;}
   for(unsigned i=0;i<bytes/4;i++){if(write)Put(route,remote+i*4,Mem(local+i*4));else Mem(local+i*4)=Gddr(route,remote+i*4);}
  }else{
   unsigned w=1;while(w<120 && PioneerQ38ConvPeer(w)!=route)w++;assert(w<120);
   auto& p=NewPeer(w-1);assert(local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
   if(write){
    assert(!p.active && p.pc==TT_Q38_PEER_WAIT_LOAD);
    assert((remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4));
    for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=Mem(local+i*4);
    if(remote==0x74100)p.PrepareAdvance();
   }else if(remote==0x74140){assert(bytes==64);for(unsigned i=0;i<16;i++)Mem(local+i*4)=p.memory.at(remote+i*4);}
   else if(remote==0xffb13140)Mem(local)=p.pc;
   else if(remote==0xffb121b0)Mem(local)=0x46800;
   else if(remote==0x83f00){
    assert(!p.active && p.pc==TT_Q38_PEER_WAIT_LOAD && bytes==(p.memory.at(0x74034)==1?256+p.RowsMachine::groups*64:256));
    for(unsigned i=0;i<bytes/4;i++)Mem(local+i*4)=i<64?p.memory.at(remote+i*4):p.buffers[4096+i-64];
   }else for(unsigned i=0;i<bytes/4;i++){
    unsigned at=remote+i*4;bool found=false;
    for(unsigned k=0;k<16;k++)if(at==0xffb20000+SrcOffset(k)){Mem(local+i*4)=p.source[k];found=true;}
    for(unsigned k=0;k<8;k++)if(at==0xffb20000+Offset(k)){Mem(local+i*4)=p.Count(k);found=true;}
    assert(found && bytes==4);
   }
  }
  Retire(bytes,write);
  if(advancePackets==counterFault)counts[0]++;
  if(advancePackets==rootSourceFault)source[11]^=1;
  if(advancePackets==manifestFault)Mem(TT_Q38_ADVANCE_MANIFEST+3*4)^=1;
 }
 void ForkActualPrefixFaults(){
  assert(advancing && !advancePackets && !forked);forked=true;fflush(nullptr);
  for(unsigned kind=0;kind<8;kind++){
   pid_t child=fork();assert(child>=0);
   if(!child){
    forkFaults=false;
    if(kind==0)source[11]^=1;
    if(kind==1)Mem(TT_Q38_LAYER_LINK_RECEIPT+5*4)^=1;
    if(kind==2)counts[0]++;
    if(kind==3)failAdvancePacket=1;
    if(kind==4)counterFault=1;
    if(kind==5)rootSourceFault=1;
    if(kind==6)manifestFault=1;
    if(kind==7)Put(manifest[9],manifest[8]+5119*4,0x7fc00001);
    assert(!RunAdvance() && unknown && !advanceJobs && !Mem(TT_Q38_ADVANCE_RECEIPT));
    if(kind<3)assert(!advancePackets);
    if(kind>=3 && kind<=5)assert(advancePackets==1);
    if(kind==6)assert(!advanceProducers);
    unsigned oldPackets=packets,reads=nocReads;Inert();assert(oldPackets==packets && reads==nocReads);
    printf("PASS ACTUAL_ADVANCE_PREFIX_FORK_FAULT%u packets%u producers%u matrices0 publication0/inert/no later IO or reset;ACTUAL prefix retained,NOT native recovery\n",kind,advancePackets,advanceProducers);
    fflush(nullptr);_exit(0);
   }
   int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
  }
 }
 bool RunAdvance(){
  while(pc!=parkPc && !(unknown && l1[Index(pc)]==0x0000006f)){
   if(pc==linkEntry)EnterLink();
   if(pc==advanceEntry && !advancing){assert(advanceCaptures==TT_Q38_ADVANCE_WORDS && !Mem(0x17ffc8));VerifyLink(false);advancing=true;
    puts("ACTUAL_ADVANCE_ENTER after actual complete layer+link/ALL5120 next input retained;NO CPU products or root relaunch");
    if(forkFaults)ForkActualPrefixFaults();}
   recentPc[recentAt++%16]=pc;StepRoot();
  }
  return !unknown;
 }
 void Diagnostic(){
  fprintf(stderr,"ADVANCE_FAILURE pc%08x ra%08x packets%u advanced%u producer%u matrix%u gatherPackets%u\n",pc,regs[1],packets,advancing,advanceProducers,advanceJobs,gatherPackets);
  for(unsigned i=0;i<16;i++)fprintf(stderr,"cached_pc%u %08x\n",i,recentPc[(recentAt+i)%16]);
  for(unsigned i=0;i<64;i++)fprintf(stderr,"cached_matrix_stat%u %08x\n",i,Mem(0x7f800+i*4));
 }
 bool ComponentCall(unsigned entry,unsigned argument=0){
  assert(!unknown && !pending);pc=entry;regs[1]=0x17ff80;regs[2]=0x70000;regs[10]=argument;
  while(pc!=0x17ff80 && !(unknown && l1[Index(pc)]==0x0000006f)){recentPc[recentAt++%16]=pc;StepRoot();}
  return !unknown;
 }
 bool Component(const char* image){
  // TEST DATA-only subroutine setup, not a fictional completed layer or
  // native producer. No LinkReceipt/terminal/ready result is supplied.
  // The full joined test separately proves the genuine preceding layer.
  advancing=true;
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++)Mem(TT_Q38_COHORT_SHADOW+i*4)=manifest[i];
  for(unsigned i=0;i<128;i++)Mem(TT_Q38_LAYER_LINK_SHADOW+i*4)=link.Descriptor()[i];
  for(unsigned w=0;w<119;w++){
   assert(NewPeer(w).pc==TT_Q38_PEER_WAIT_LOAD && !NewPeer(w).memory.at(0x74144));
   Mem(TT_Q38_COHORT_SEQUENCE+w*4)=0;
   for(unsigned i=0;i<16;i++)Mem(0x7d800+w*64+i*4)=manifest[TT_Q38_COHORT_SOURCE_WORD+(w+1)*16+i];
  }
  for(unsigned i=0;i<5120;i++)Put(manifest[9],manifest[8]+i*4,Bits(float(int((i*37)%255)-127)*.125f));
  if(!ComponentCall(Symbol(image,"q38_resident_advance_capture")) || !ComponentCall(Symbol(image,"_ZN7AdvanceL8QuantizeEv")))return false;
  const auto* m=advancePlan.Descriptor();std::array<uint32_t,64> d={};
  d[0]=TT_Q38_PROJECTION_GATHER_MARKER;d[1]=1;d[2]=67108864;d[3]=m[6];d[4]=m[7];d[5]=20;d[6]=m[9];d[8]=m[15];d[9]=129;
  d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;d[13]=10240;d[14]=640;d[15]=8;
  for(unsigned i=16;i<40;i++)d[i]=m[i];d[40]=m[4];d[41]=m[5];d[42]=m[3];d[43]=5;
  for(unsigned i=0;i<64;i++)Mem(0x74200+i*4)=d[i];
  if(!ComponentCall(Symbol(image,"q38_resident_advance_gather_once")))return false;
  for(unsigned role=0;role<4;role++)if(!ComponentCall(Symbol(image,"_ZN7AdvanceL6MatrixEj"),role))return false;
  assert(advanceProducers==20 && advanceJobs==244 && !Mem(TT_Q38_ADVANCE_RECEIPT));
  for(unsigned role=0;role<4;role++){
   const auto* r=PioneerQ38ResidentAdvanceRecord(advancePlan.Descriptor(),role);
   assert(proofStores[role]==r[2]*64);
   for(unsigned row=0;row<r[4];row++)assert(cover[role][row]==1 && Gddr(manifest[9],manifest[8]+r[6]+row*4)==observed[role][row]);
  }
  puts("PASS ACTUAL advance SUBROUTINE components/20real QPD1 producers/ALL4matrices244jobs/ALL16480rows arithmetic checked/SAME119peers/no numerical feedback;synthetic raw input and descriptor DATA only,NOT completed-prefix/native/full-model qualification");return true;
 }
 void VerifyAdvance(){
  assert(advancing && !unknown && !pending && advanceJobs==244 && advanceProducers==20 && gatherPackets==22 && matrixInputPackets==8
   && Mem(0x17ffc8)==TT_Q38_ADVANCE_MARKER && Mem(0x17ffc0)==donePc && Mem(0x17ffc4)==5120);
  assert(Mem(TT_Q38_ADVANCE_RECEIPT)==TT_Q38_ADVANCE_RECEIPT_MARKER && Mem(TT_Q38_ADVANCE_RECEIPT+49*4)==647);
  std::array<uint32_t,512> receipt;for(unsigned i=0;i<512;i++)receipt[i]=Mem(TT_Q38_ADVANCE_RECEIPT+i*4);
  std::array<uint32_t,8> start;for(unsigned i=0;i<8;i++)start[i]=Mem(TT_Q38_LAYER_LINK_RECEIPT+(56+i)*4);
  assert(PioneerQ38ResidentAdvanceReceipt(receipt.data(),advancePlan.Descriptor(),link.Descriptor(),manifest.data(),start.data(),counts,command));
  unsigned refusals=0;
  for(unsigned i=0;i<512;i++){
   if(i==48 || (i>=96 && i<352 && (i-96)%64>=57))continue; // bounded variable polls/opaque cycle deltas,not authority
   receipt[i]^=1;assert(!PioneerQ38ResidentAdvanceReceipt(receipt.data(),advancePlan.Descriptor(),link.Descriptor(),manifest.data(),start.data(),counts,command));receipt[i]^=1;refusals++;
  }
  assert(refusals==483);puts("PASS actual advance complete-root traffic/gather/4matrix receipts/ALL119sequences/483fixed-receipt-word corruptions refused;DATA ONLY,not cleanup/native authority");
  for(unsigned role=0;role<4;role++){
   const auto* r=PioneerQ38ResidentAdvanceRecord(advancePlan.Descriptor(),role);
   assert(proofStores[role]==r[2]*64);
   for(unsigned row=0;row<r[4];row++)assert(cover[role][row]==1 && Gddr(manifest[9],manifest[8]+r[6]+row*4)==observed[role][row]);
  }
  for(unsigned w=0;w<119;w++){
   auto& p=NewPeer(w);unsigned expected=peerSequence[w]+unsigned(w<20)+2+2*unsigned(w<3);
   assert(!p.active && !p.pending && p.pc==TT_Q38_PEER_WAIT_LOAD && p.memory.at(0x74144)==expected
    && Mem(TT_Q38_ADVANCE_RECEIPT+(378+w)*4)==expected);
  }
  assert(minStack>=0x6e000);
 }
};
void AdvancePeer::PrepareAdvance(){
 assert(advanceFabric && !active && pc==TT_Q38_PEER_WAIT_LOAD && !pending);ClearTensorState();
 uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
 NextPackets();for(unsigned i=0;i<8;i++)countBase[i]=independentBefore[i]=physical[i];
 assert(memory.at(0x74030)==launched+1 && memory.at(0x74100)==launched+1);
 advanceSequence=launched=memory.at(0x74100);advance=true;ffn=gdnActive=convActive=false;
 unsigned op=memory.at(0x74034);assert(op==1 || op==5);activation=outputActive=op==5;
 activationPackets=commandReads=activatedWrites=nativeWrites=lineWrites=scaleWrites=0;
 if(activation){for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(0x74200+i*4);active=true;return;}
 advanceRole=advanceFabric->activeRole;const auto* r=PioneerQ38ResidentAdvanceRecord(advanceFabric->advancePlan.Descriptor(),advanceRole);
 const auto* part=r+64+(logicalWorker-1)*8;assert(logicalWorker<=r[2]);
 type=8;inputs=5120;count=32;RowsMachine::groups=part[2];base=part[0];vector.resize(inputs);scales.resize(inputs/32);
 tile.assign(576,0);tiles.assign(RowsMachine::groups*160,tile);
 for(unsigned t=0;t<tiles.size();t++)for(unsigned row=0;row<16;row++){
  unsigned global=r[1016+logicalWorker-1]+(t/160)*16+row;uint16_t half=uint16_t(0x0800+((global+advanceRole)%4)*0x80);
  tiles[t][row]=uint8_t(half);tiles[t][16+row]=uint8_t(half>>8);
  for(unsigned k=0;k<32;k++)tiles[t][(k+2)*16+row]=uint8_t(int((t*17+global*19+k*37+advanceRole*11)%255)-127);
 }
 root.clear();packets.clear();packets.push_back({0xa0000,129,0xa0000,10240,-1});
 for(unsigned i=0;i<10240;i+=4)root[0xa0000+i]=advanceFabric->Mem(0xa0000+i);
 packets.push_back({0xab000,129,0x94000,640,-1});for(unsigned i=0;i<640;i+=4)root[0xab000+i]=advanceFabric->Mem(0xab000+i);
 inputReads=2;for(unsigned t=0;t<tiles.size();t++)packets.push_back({base+t*576,part[4],0x90000,576,int(t)});
 for(unsigned i=0;i<inputs;i++)vector[i]=uint16_t(root.at(0xa0000+i/2*4)>>((i%2)*16));
 for(unsigned i=0;i<scales.size();i++)scales[i]=root.at(0xab000+i*4);
 Poison();PoisonTail();for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;active=true;
}
void AdvancePeer::SubmitActivation(){
 if(!advance){LayerPeer::SubmitActivation();return;}
 ClearTensorState();assert(activation && outputActive && activationPackets<3);unsigned p=activationPackets++;
 bool write=p!=0;unsigned local=p==0?0x90000:p==1?0x90c00:0x90e00,bytes=p==0?1024:p==1?512:128;
 unsigned remote=descriptor[4]+descriptor[p==0?10:p==1?12:13];
 const uint32_t expect[]={write?local:remote,0,write?selfRoute:descriptor[8],write?remote:local,0,write?descriptor[8]:selfRoute,0,write?0x212U:0,bytes,0};
 for(unsigned i=0;i<10;i++)assert(command[i]==expect[i]);
 for(unsigned i=0;i<bytes/4;i++){
  if(!write){uint32_t v=advanceFabric->Gddr(descriptor[8],remote+i*4);memory[local+i*4]=gate[i]=v;}
  else{uint32_t v=memory.at(local+i*4);advanceFabric->Put(descriptor[8],remote+i*4,v);(p==1?native.data():line.data())[i]=v;}
 }
 unsigned at=write?4:0;countBase[at]++;countBase[at+1]+=bytes/64;countBase[at+3]++;countBase[2]++;countBase[6]++;
}
void AdvancePeer::VerifyProjection(){
 assert(activationPackets==3 && nativeWrites==128 && lineWrites==40 && !activatedWrites);
 float values[256];for(unsigned i=0;i<256;i++)values[i]=Float(gate[i]);PioneerQ38ProjectionInput ref;ProjectionInputReference(8,values,ref);
 for(unsigned i=0;i<128;i++)assert(native[i]==ref.operand[i]);for(unsigned i=0;i<8;i++)assert(line[i]==ref.scale[i]);
 assert(PioneerQ38ProjectionInputLine(line.data(),descriptor.data()));
}
void AdvancePeer::StepBlock(){
 if(!advance){LayerPeer::StepBlock();return;}
 CohortPeer::StepBlock();if(!active || pc!=TT_Q38_PEER_WAIT_LOAD || memory.at(0x74144)!=advanceSequence)return;
 ClearTensorState();assert(!pending && !memory.at(0x74148));
 if(activation){VerifyProjection();advanceFabric->advanceProducers++;}
 else{
  assert(requests==packets.size() && CorrectRows() && !dotProbeReads);advanceFabric->advanceJobs++;
  const auto* r=PioneerQ38ResidentAdvanceRecord(advanceFabric->advancePlan.Descriptor(),advanceRole);unsigned first=r[1016+logicalWorker-1];
  for(unsigned i=0;i<RowsMachine::groups*16;i++){assert(!advanceFabric->cover[advanceRole][first+i]++);advanceFabric->observed[advanceRole][first+i]=buffers[4096+i];}
  if(logicalWorker==1 || logicalWorker==119)printf("ACTUAL_ADVANCE_MATRIX role%u worker%u seq%u;NOT_NATIVE_TIME\n",r[0],logicalWorker,advanceSequence);
 }
 active=false;
}
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && (!strcmp(argv[3],"--faults") || !strcmp(argv[3],"--component"))));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 puts("BEGIN ACTUAL full layer+link+shared quantization+FOUR next-layer matrices;SAME119peer programs, synthetic byte endpoints ONLY");
 OutputParts geometry(0,0);AdvanceFabric f(argv[1],argv[2],geometry);f.forkFaults=argc==4 && !strcmp(argv[3],"--faults");
 if(argc==4 && !strcmp(argv[3],"--component")){if(f.Component(argv[1]))return 0;f.Diagnostic();return 1;}
 if(!f.RunAdvance()){f.Diagnostic();return 1;}
 f.VerifyAdvance();f.Inert();
 printf("PASS ACTUAL uninterrupted layer->link->next projections/ONE20worker Q8 producer group/244 actual matrix jobs/ALL16480 resident rows checked/ALL119 private sequences retained/maxStack%u;NO host product feedback/reset/relaunch;NOT native/original-model/full64layer/token-rate qualification\n",0x70000-f.minStack);
}
