/* SPDX-License-Identifier: MIT */
// Run actual RV32, Tensix and NoC instructions for the isolated Q8T4 role.
#define PIONEER_Q38_TWO_STREAM_MODEL_ONLY
#include "pioneer_q38_two_token_q4_stream_test.cpp"
#define PIONEER_Q38_VECTOR_HAS_ROWS_MACHINE
#define PIONEER_Q38_VECTOR_ROWS_MODEL_ONLY
#include "pioneer_q38_vector_rows_image_test.cpp"
#define PIONEER_Q38_VECTOR_Q4_METADATA_MODEL_ONLY
#include "pioneer_q38_vector_q4_metadata_test.cpp"
#include <fstream>

using VectorStream=VectorRows<TwoStream,false,false,true>;
struct TwoVector:VectorStream {
 enum Scope {None, Begin, Metadata, Affine};
 struct DstHazard {};
 unsigned entry[4]={},ret=0,init=0,prefixes=0,panels=0,dotLoads=0,clears=0;
 unsigned exportWords=0;
 Scope scope=None;
 MetadataMachine math;
 bool omitScaleInit=false,omitSecondClear=false,duplicateActivation=false,badFinal=false;

 TwoVector(const char* image,const StreamMachine& a,const StreamMachine& b,unsigned bank)
  :VectorStream(image,a,b,bank){
  memory[L::Descriptor]=L::VectorMagic;
  std::ifstream records(std::string(image)+"/program.symbols");
  std::string address,kind,name;
  while(records>>address>>kind>>name){
   const auto index=name=="pair_vector_begin"?Begin:name=="pair_vector_block"?Metadata:name=="pair_vector_affine"?Affine:None;
   if(index!=None){assert(!entry[index]);entry[index]=unsigned(std::stoul(address,nullptr,16));}
  }
  assert(entry[Begin] && entry[Metadata] && entry[Affine]);
 }
 unsigned MatrixPhasesPerPanel()const override{return 2;}
 unsigned MatrixFidelityStep()const override{return 2;}
 bool AdditionalZeroInitRow(unsigned row)const override{return scope==Begin && row>=112 && row<=176 && !(row&15);}

 void StepBlock()override{
  for(unsigned i=1;i<4;i++)if(pc==entry[i]){
   assert(scope==None && !pending && !rawPending && !ownerA && !ownerB && drained);
   assert(!sources[0] && !sources[1] && unpackBank==mathBank);
   scope=Scope(i);ret=regs[1];math=MetadataMachine{};
   if(scope==Affine && (clears!=3 || matrixInstructions!=2*(panels+1)))throw DstHazard{};
  }
  VectorStream::StepBlock();
  if(scope!=None && pc==ret){
   assert(drained && !pending && !rawPending && thread[5]==4 && !ownerA && !ownerB);
   if(scope==Begin)assert(init==7);
   if(scope==Metadata)assert(prefixes==responses);
   if(scope==Affine){assert(dotLoads==2*(panels+1));panels++;clears=0;}
   scope=None;ret=0;
  }
 }

 void Instruction(uint32_t op)override{
  unsigned code=op>>24;
  if(scope==None || code<0x70 || code>0x90){
   if(op==0x42000091){
    static constexpr unsigned rows[]={64,96,112,128,144,160,176};
    assert(scope==Begin && init<7 && config[84]==0x400000+(rows[init]+4)*16);
    init++;
    if(omitScaleInit && init==3)return;
   }
   if(op==0x42000081){
    assert(scope==Metadata && (config[76]+1)*16==L::Tile && prefixes+1==responses);
    prefixes++;
   }
   if(op==0x10080004 || op==0x10080005){
    assert(scope==None && clears==(op==0x10080004?0U:1U));
    if(omitSecondClear && op==0x10080005)return;
    clears|=op==0x10080004?1:2;
   }
   if(op==0x10180000)assert(scope==Begin && !matrixInstructions && !init);
   VectorStream::Instruction(op);
   if(op==0x428000c1 && duplicateActivation)memcpy(b[1],b[0],sizeof(b[0]));
   if(scope!=None)math.cycle++;
   return;
  }
  assert(!pending && !rawPending && !ownerA && !ownerB && !sources[0] && !sources[1]);
  assert(thread[5]==4 && config[1]==0xa03c8000 && config[220]==0);
  drained=false;vectorOps++;
  if(op==0x8a000002){enabled=true;math.cycle++;return;}
  assert(enabled);
  if(code==0x70 || code==0x72){
   // Same logical->physical FP32 split/merge as the qualified vector
   // unpack observer. Require the complete band initialization first.
   const unsigned r=(op>>20)&15,mode=(op>>16)&15,at=op&1023;
   assert(r<8 && !(op&0xfc01));
   if(mode==4){assert(scope==Affine && code==0x70 && !r && (at&~3U)==32);dotLoads++;}
   for(unsigned lane=0;lane<32;lane++){
    const unsigned row=(at&~3U)+lane/8,col=(lane%8)*2+bool(at&2);
    if(code==0x70){
     if(mode==13){
      assert(scope==Metadata && row<16 && defined[row] && wrote[row][col]);
      math.lanes[r][lane]=BlackholeRawSfpLoad(physical[row][col],13);
     }else if(mode==4){
      assert(valid[row]);
      const int d=dstInt[row][col];
      assert(d>=-30720 && d<=30720 && (row<34 || !d));
      math.lanes[r][lane]=d>0?unsigned(d):0x80000000U|unsigned(-d);
     }else{
      const unsigned group=128+((panels%16)/2)*8;
      assert(mode==3 && ((scope==Metadata && row>=184 && row<192) ||
       (scope==Affine && ((!r) ? false:r==1?row>=group && row<group+4:
        r==2?row>=group+4 && row<group+8:r==3?row>=112 && row<116:
        r==4?row>=116 && row<120:r==5 && row>=108 && row<112))));
      math.lanes[r][lane]=FloatDst(row,col);
     }
    }else{
     assert(mode==3 && ((scope==Begin && r==7 && row>=108 && row<112)
      || (scope==Metadata && ((row>=128 && row<192) || (row>=112 && row<116)))
      || (scope==Affine && !r && ((row>=108 && row<112) || (row>=116 && row<120)))));
     if(!defined[Adj(row)] || !wrote[Adj(row)][col] || !wrote[Adj(row)+8][col])throw DstHazard{};
     StoreFloatDst(row,col,math.Read(r,lane));
    }
   }
   if(code==0x70)math.ready[r]=math.cycle+1;
   math.cycle++;
  }else if(op==0x8c000000){
   uint32_t old[8][32];
   for(unsigned r=0;r<8;r++)for(unsigned l=0;l<32;l++)old[r][l]=math.Read(r,l);
   for(unsigned b:{0U,4U})for(unsigned r=0;r<4;r++)for(unsigned g=0;g<4;g++)for(unsigned c=0;c<8;c++)
    math.lanes[b+r][g*8+c]=old[b+g][r*8+c];
   for(auto& r:math.ready)r=math.cycle+1;
   math.cycle++;
  }else if(code==0x90 && scope==Affine){
   assert(op==0x90000000);
   for(unsigned lane=0;lane<32;lane++){
    const uint32_t sm=math.Read(0,lane);
    const int mag=int(sm&0x7fffffff);
    assert(sm!=0x80000000 && mag<=30720);
    math.lanes[0][lane]=Bits(float(sm>>31?-mag:mag));
   }
   math.ready[0]=math.cycle+1;math.cycle++;
  }else math.Op(op);
  assert(math.okay);
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0xffbd8000,64*512)){
   assert(Range(at,0xffbd9b00,128) && drained && !rawPending && !pending && !config[3]
    && scope==None && panels==inputs/16 && requests==responses && !published);
   const unsigned index=(at-0xffbd9b00)/4;assert(index==exportWords++);
   return badFinal && index==31?0x7fc00000:FloatDst(108+index/16,index%16);
  }
  return VectorStream::Load(at);
 }
 void Store8(uint32_t at,uint8_t value)override{
  // -Os lowered the Q4 span's local bool to a byte store. No byte store
  // to inputs, scratch, MMIO or output has been granted.
  assert(Range(at,0x6e000,8192) && at>=regs[2] && !value);
  const uint32_t word=at&~3U;
  const auto old=memory.find(word);
  const unsigned shift=(at&3U)*8;
  VectorStream::Store(word,(old==memory.end()?0xdeadbeefU:old->second)&~(255U<<shift));
 }
 bool Run(){return RunPair(L::VectorMagic,L::VectorFault);}
 bool Match(const StreamMachine& a,const StreamMachine& b)const{
  for(unsigned n=0;n<16;n++)if(buffers[4096+n]!=a.buffers[4096+n] || buffers[4112+n]!=b.buffers[4096+n])return false;
  return true;
 }
};

int main(int argc,char** argv){
 assert(argc==3 || argc==4);(void)&Round;
 unsigned reads=0,cases=0,negatives=0;
 for(unsigned width:{5120U,6144U,17408U}){
  auto a=std::make_unique<StreamMachine>(argv[1],12,width,1);
  auto b=std::make_unique<StreamMachine>(argv[1],12,width,191);
  b->tiles=a->tiles;
  for(unsigned pos=0;pos<b->tiles.size();pos++){
   b->scales[pos]=Bits(pos%2?0x1p-32f:.0078125f);
   b->memory[L::Scale0+pos*4]=b->scales[pos];
  }
  auto pair=std::make_unique<TwoVector>(argv[2],*a,*b,width%7);
  const auto before=pair->memory;pair->delay=7;
  assert(a->RunStream() && a->CorrectStream() && b->RunStream() && b->CorrectStream());
  assert(pair->Run() && pair->Match(*a,*b));
  assert(pair->init==7 && pair->prefixes==pair->requests && pair->requests*2==a->requests+b->requests
   && pair->panels==width/16 && pair->matrixInstructions==a->matrixInstructions/2
   && pair->bUnpacks==pair->panels && pair->rawUnpacks==7+pair->prefixes+pair->panels
   && pair->moveInstructions==pair->panels*4 && !pair->aUnpacks && !pair->floatOps
   && pair->exportWords==32 && pair->published==32 && !pair->outputReads);
  for(unsigned n=32;n<4096;n++)assert(pair->buffers[4096+n]==0x7fc00000);
  for(const auto& [at,v]:before)if(TwoVector::Range(at,L::Descriptor,L::Words*4) ||
    TwoVector::Range(at,L::Input0,width*2) || TwoVector::Range(at,L::Input1,width*2) ||
    TwoVector::Range(at,L::Scale0,unsigned(pair->tiles.size()*4)) ||
    TwoVector::Range(at,L::Scale1,unsigned(pair->tiles.size()*4)))assert(pair->memory.at(at)==v);
  printf("PASS Q8T4 K%u tensor reads%u/%u matrix commands%u/%u export32, 32 exact results\n",
   width,pair->requests,a->requests+b->requests,pair->matrixInstructions,a->matrixInstructions+b->matrixInstructions);
  reads+=pair->requests;cases++;
  if(width!=5120)continue;
  for(unsigned i=0;i<2;i++){
   auto missing=std::make_unique<TwoVector>(argv[2],*a,*b,1);
   missing->omitScaleInit=i==0;missing->omitSecondClear=i==1;
   bool hazard=false;try{(void)missing->Run();}catch(TwoVector::DstHazard&){hazard=true;}
   assert(hazard && !missing->published && !missing->exportWords);negatives++;
  }
  for(unsigned i=0;i<6;i++){
   auto fault=std::make_unique<TwoVector>(argv[2],*a,*b,1);
   if(i==0)fault->memory[L::Descriptor]=L::Magic;
   if(i==1)fault->faultAt=10;
   if(i==2){fault->fabricFault=3;fault->when=20;}
   if(i==3)fault->tiles[0][16]|=0x7cU;
   if(i==4)fault->badFinal=true;
   if(i==5)fault->duplicateActivation=true;
   const bool done=fault->Run();
   if(i!=5)assert(!done && !fault->published);
   else assert(done && fault->published==32 && !fault->Match(*a,*b));
   negatives++;
  }
 }
 printf("PASS linked vector pair: %u widths, %u shared reads, %u negative controls; command simulation, not silicon timing\n",cases,reads,negatives);
 if(argc==4){
  FILE* file=fopen(argv[3],"rb");
  PioneerQwen38Gguf model;assert(file && model.Open(file));
  unsigned actual=0;
  for(unsigned width:{5120U,6144U,17408U}){
   unsigned index=0;
   const auto& tensors=model.Tensors();
   while(index<tensors.size() && (tensors[index].inputs!=width || tensors[index].type!=12 || tensors[index].outputs<16))index++;
   if(index==tensors.size())continue;
   const auto& t=tensors[index];
   for(unsigned row:{0U,unsigned(t.outputs)-16}){
    auto a=std::make_unique<StreamMachine>(argv[1],12,width,1);
    auto b=std::make_unique<StreamMachine>(argv[1],12,width,191);
    a->BindModel(model,index,row);b->tiles=a->tiles;
    auto pair=std::make_unique<TwoVector>(argv[2],*a,*b,width%7);pair->delay=7;
    assert(a->RunStream() && a->CorrectStream() && b->RunStream() && b->CorrectStream());
    assert(pair->Run() && pair->Match(*a,*b) && pair->published==32 && pair->exportWords==32
     && pair->requests==a->requests && pair->responses==pair->requests && model.Stable());
    printf("PASS real Q4 tensor %s K%u rows%u..%u; 32 exact outputs\n",t.name.c_str(),width,row,row+15);
    actual++;
   }
  }
  assert(actual && model.Stable());fclose(file);
  printf("PASS %u real weight shards; synthetic activations and command observer, no hardware timing\n",actual);
 }
}
