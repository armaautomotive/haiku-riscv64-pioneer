/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include <cassert>
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
using namespace Q38CpuReferenceMath;
struct ReferenceRowsProbe:Q38OriginalRows {
 std::vector<Q38OriginalTensor> tensors;uint64_t epoch=1;bool live=true;unsigned reads=0;
 mutable unsigned describes=0;unsigned refuseDescription=0;
 ReferenceRowsProbe(){
  const char* common[]={"attn_norm.weight","post_attention_norm.weight","ffn_gate.weight","ffn_up.weight","ffn_down.weight"};
  const unsigned ci[]={5120,5120,5120,5120,17408},co[]={1,1,17408,17408,5120},ct[]={0,0,12,12,12};
  const char* delta[]={"attn_gate.weight","attn_qkv.weight","ssm_a","ssm_dt.bias","ssm_norm.weight","ssm_alpha.weight","ssm_beta.weight","ssm_conv1d.weight","ssm_out.weight"};
  const unsigned di[]={5120,5120,48,48,128,5120,5120,4,6144},dout[]={6144,10240,1,1,1,48,48,10240,5120},dt[]={8,8,0,0,0,8,8,0,8};
  const char* attention[]={"attn_q.weight","attn_k.weight","attn_v.weight","attn_q_norm.weight","attn_k_norm.weight","attn_output.weight"};
  const unsigned ai[]={5120,5120,5120,256,256,6144},ao[]={12288,1024,1024,1,1,5120},at[]={8,8,8,0,0,14};
  auto add=[&](std::string name,unsigned in,unsigned out,unsigned type,unsigned rank){
   unsigned width=type==8?32:type?256:1,packed=type==8?34:type==12?144:type==14?210:4;
   tensors.push_back({name,type,rank,in,out,uint64_t(in/width)*packed*out,uint64_t(tensors.size())<<32});
  };
  add("token_embd.weight",5120,248320,12,2);add("output.weight",5120,248320,14,2);add("output_norm.weight",5120,1,0,1);
  for(unsigned layer=0;layer<64;layer++){
   std::string prefix="blk."+std::to_string(layer)+".";
   for(unsigned i=0;i<5;i++)add(prefix+common[i],ci[i],co[i],ct[i],ct[i]?2:1);
   if(layer%4==3)for(unsigned i=0;i<6;i++)add(prefix+attention[i],ai[i],ao[i],at[i],at[i]?2:1);
   else for(unsigned i=0;i<9;i++)add(prefix+delta[i],di[i],dout[i],dt[i],dt[i] || i==7?2:1);
  }
  assert(tensors.size()==851);
 }
 bool Ready()const override{return live;}
 uint64_t Epoch()const override{return live?epoch:0;}
 bool Describe(unsigned i,Q38OriginalTensor& out)const override{
  describes++;if(!live || i>=tensors.size() || (refuseDescription && describes==refuseDescription))return false;out=tensors[i];return true;
 }
 bool ReadOriginal(unsigned i,uint64_t at,unsigned bytes,void* out,unsigned capacity)override{
  assert(live && i<851 && out && bytes && bytes<=1048576 && capacity>=bytes && at<=tensors[i].bytes && bytes<=tensors[i].bytes-at);reads++;
  std::memset(out,0,bytes);const auto& t=tensors[i];
  if(!t.type){uint32_t word=t.name.find("ssm_a")!=std::string::npos?Bits(-1.f):t.name.find("norm.weight")!=std::string::npos?Bits(1.f):0;
   auto* p=static_cast<uint8_t*>(out);for(unsigned b=0;b<bytes;b++)p[b]=uint8_t(word>>((b%4)*8));}
  return true;
 }
};
static void TestHalf(){
 unsigned count=0;
 for(unsigned h=0;h<65536;h++)if((h>>10&31)!=31){
  uint16_t bits=uint16_t(h);_Float16 native;std::memcpy(&native,&bits,2);assert(Bits(Half(h))==Bits(float(native)));
  assert(HalfRne(float(native))==bits);count++;
 }
 for(float x:{0.f,-0.f,0x1p-25f,0x1.8p-24f,0x1.ffcp15f,65520.f,-65520.f,0x1.001p-14f}){
  _Float16 h=_Float16(x);uint16_t bits;std::memcpy(&bits,&h,2);assert(HalfRne(x)==bits);
 }
 printf("PASS independent half conversion/%u finite binary16 values/native host half+boundary RNE checks\n",count);
}
static void TestMatrix(){
 unsigned cases=0;
 for(unsigned type:{8,12,14})for(unsigned inputs:{256,512}){
  unsigned width=type==8?32:256,packed=type==8?34:type==12?144:210,blocks=inputs/width,rowBytes=blocks*packed;
  std::vector<float> input(inputs),expected(16),actual;std::vector<uint8_t> raw(16*rowBytes);
  for(unsigned i=0;i<inputs;i++)input[i]=float(int((i*19+3)%255)-127)*0x1p-10f;
  // EACH Q8 32-value block needs the same exact power-of-two scale.
  // A single maximum for the whole row would introduce real activation
  // quantization error and invalidate the independent exact-dot fixture.
  for(unsigned at=0;at<inputs;at+=32)input[at]=-127.f*0x1p-10f;
  for(unsigned row=0;row<16;row++){
   double sum=0;
   for(unsigned block=0;block<blocks;block++){
    auto* p=raw.data()+row*rowBytes+block*packed;p[type==14?208:0]=0;p[type==14?209:1]=0x28; // d=2^-5
    if(type==12){p[2]=0;p[3]=0x24;for(unsigned g=0;g<4;g++){p[4+g]=uint8_t(1+g%3);p[8+g]=uint8_t(1+g%2);p[12+g]=uint8_t((1+(g+4)%3)|((1+(g+4)%2)<<4));}}
    if(type==14)for(unsigned g=0;g<16;g++)p[192+g]=uint8_t(g&1?-2:1);
    for(unsigned i=0;i<width;i++){
     int quant=type==8?(i==0?-128:i==1?127:int((i*7+row*3+block)%127)-63):type==12?int((i+row+block)%16):int((i*7+row*3+block)%64)-32;
     if(type==8)p[2+i]=uint8_t(quant);
     else if(type==12){unsigned at=16+i/64*32+i%32,shift=i/32%2*4;p[at]|=uint8_t(unsigned(quant)<<shift);}
     else{unsigned h=i/128,g=i%128/32,l=i%32,v=unsigned(quant+32);p[h*64+g%2*32+l]|=uint8_t((v&15)<<((g/2)*4));p[128+h*32+l]|=uint8_t((v>>4)<<(g*2));}
     unsigned group=i/32;float weight=type==8?float(quant)*0x1p-5f:type==12?float((1+group%3)*quant)*0x1p-5f-float(1+group%2)*0x1p-6f:float(int(i/16&1?-2:1)*quant)*0x1p-5f;
     sum+=double(weight)*input[block*width+i];
    }
   }
   expected[row]=float(sum);
  }
  auto read=[&](uint64_t at,unsigned bytes,void* out){assert(at+bytes<=raw.size());std::memcpy(out,raw.data()+at,bytes);return true;};
  bool okay=Matrix(type,inputs,16,input,actual,read);
  if(!okay || actual!=expected){
   fprintf(stderr,"MATRIX_REFERENCE_DIAGNOSTIC type%u inputs%u returned%u rows%zu\n",type,inputs,unsigned(okay),actual.size());
   for(unsigned row=0;row<actual.size();row++)if(actual[row]!=expected[row])fprintf(stderr," row%u actual%08x %.9g expected%08x %.9g\n",row,Bits(actual[row]),actual[row],Bits(expected[row]),expected[row]);
  }
  assert(okay && actual==expected);
  auto before=actual;assert(!Matrix(type,inputs,16,input,actual,[](uint64_t,unsigned,void*){return false;}) && actual==before);cases++;
 }
 printf("PASS independent original-row matrix/%u Q8Q4Q6 cases/all16rows/scalar decoded powers-of-two dot exact/atomic read refusal\n",cases);
}
static void TestState(){
 std::array<float,128> q={},k={},value,out;std::vector<float> state(16384);q[0]=k[0]=1;
 std::array<float,128> closed={};
 for(unsigned token=0;token<8;token++){
  for(unsigned i=0;i<128;i++)value[i]=float(int(i%17)-8)*.125f+float(token)*.25f;
  assert(Gdn(q.data(),k.data(),value.data(),.5f,.25f,state.data(),out.data()));
  for(unsigned i=0;i<128;i++){closed[i]=closed[i]*.5f+(value[i]-closed[i]*.5f)*.25f;assert(state[i]==closed[i] && out[i]==closed[i]*0.0883883461356163f);}
  for(unsigned i=128;i<state.size();i++)assert(state[i]==0);
 }
 std::vector<float> qg(12288),key(1024),v(1024),norm(256,1),cacheK,cacheV,result;
 std::array<std::vector<float>,3> values;
 for(unsigned token=0;token<3;token++){
  for(unsigned i=0;i<1024;i++){key[i]=i%256==0?1.f:0;v[i]=float(i%13)*.25f+float(token);}
  values[token]=v;assert(Attention(qg,key,v,norm,norm,token,cacheK,cacheV,result));float probability=Half(HalfRne(1.f/float(token+1)));
  for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){double sum=0;for(unsigned t=0;t<=token;t++)sum+=double(probability)*values[t][h/6*256+i];assert(result[h*256+i]==float(sum)*.5f);}
  assert(cacheK.size()==size_t(token+1)*1024 && cacheV.size()==cacheK.size());
 }
 puts("PASS independent GDN/8 retained nonzero steps/closed-form basis solution/all16384state+128output;attention3positions/all24heads/uniform half-softmax/append-only original V history");
}
static void TestRms(){
 unsigned rows=0;
 for(unsigned sample=0;sample<64;sample++){
  std::vector<float> input(5120),weight(5120),actual;
  for(unsigned i=0;i<5120;i++){input[i]=float(std::sin(i*.017+sample*.11))*float(sample+1);weight[i]=.5f+float(i%13)*.03125f;}
  if(sample==1){input.assign(5120,0x1p26f);weight.assign(5120,0x1p-50f);input[0]=0x1p-50f;}
  if(sample==2)input[3]=66265.703125f;
  if(sample==3)input.assign(5120,0);
  assert(Rms(input,weight,actual));
  long double sum=0;for(float x:input)sum+=static_cast<long double>(float(x*x));
  float mean=float(sum/5120),root=float(std::sqrt(double(mean+1e-6f))),scale=float(1.0/double(root));
  for(unsigned i=0;i<5120;i++)assert(Bits(actual[i])==Bits(float(float(input[i]*scale)*weight[i])));rows++;
 }
 std::vector<float> input(5120,1),weight(5120,1),output(1,42);auto before=output;
 for(float bad:{Float(1),std::numeric_limits<float>::infinity(),0x1.000002p26f}){input[0]=bad;assert(!Rms(input,weight,output) && output==before);}
 input[0]=1;weight[0]=65536;assert(!Rms(input,weight,output) && output==before);
 printf("PASS independent raw FP32 RMS/%u rows/all5120 bits/long-double FLOAT-square reduction+host sqrt/div/captured legacy-overflow+2^26+minimum-product edge/domain refusals atomic\n",rows);
}
static void TestBind(){
 ReferenceRowsProbe good;Q38CpuModelReference reference;assert(reference.Bind(good) && reference.Ready() && !reference.Bind(good) && !good.reads);
 std::vector<uint32_t> snapshot={0x13579bdf};const auto initial=snapshot;
 for(unsigned completed:{0U,1U,2U,3U,UINT32_MAX})for(unsigned layer:{0U,3U,63U,64U,UINT32_MAX})for(unsigned kind:{0U,1U,2U,3U,4U,UINT32_MAX}){
  assert(!reference.Snapshot(completed,layer,kind,snapshot) && snapshot==initial && reference.Ready() && !good.reads);
  assert(!reference.SnapshotEos(layer,kind,snapshot) && snapshot==initial && reference.Ready() && !good.reads);
 }
 Q38CpuTokenTrace out;out.hidden={0xa5a5a5a5};good.epoch++;
 assert(!reference.Step(0,out) && !reference.Ready() && !good.reads && out.hidden==std::vector<uint32_t>{0xa5a5a5a5});
 good.epoch--;assert(!reference.Step(0,out) && !reference.Ready());
 for(unsigned bad=0;bad<5;bad++){
  ReferenceRowsProbe source;if(bad==0)source.tensors[850].outputs++;if(bad==1)source.tensors[850].name=source.tensors[0].name;
  if(bad==2)source.epoch=0;if(bad==3)source.live=false;if(bad==4)std::fesetround(FE_DOWNWARD);
  Q38CpuModelReference ref;assert(!ref.Bind(source) && !source.reads);std::fesetround(FE_TONEAREST);
 }
 puts("PASS independent whole-model binding/all851 exact shapes/names/last-tensor+duplicate+epoch+source+rounding refusals/late epoch loss poisons reference/no retry/CPU output unchanged");
 puts("PASS CPU-private state snapshot/pre-Step+invalid position/layer/kind requests refuse atomically without learned reads or reference mutation");
}
int main(int argc,char** argv){
 assert(argc==1 || (argc==2 && !std::strcmp(argv[1],"--full-zero")));setvbuf(stdout,nullptr,_IOLBF,0);
 TestHalf();TestMatrix();TestRms();TestState();TestBind();
 if(argc==2){
  ReferenceRowsProbe source;Q38CpuModelReference reference;assert(reference.Bind(source));
  for(unsigned position=0;position<3;position++){
   Q38CpuTokenTrace trace;assert(reference.Step(position==0?248319:position==1?17:0,trace));
   assert(trace.position==position && trace.selected==0 && trace.runnerUp==1 && trace.hidden.size()==65*5120 && trace.logits.size()==248320);
   for(auto b:trace.hidden)assert(!(b&0x7fffffff));for(auto b:trace.logits)assert(!(b&0x7fffffff));
   if(position>=1){
    unsigned records=0;const unsigned reads=source.reads;
    for(unsigned layer=0;layer<64;layer++)for(unsigned kind=0;kind<4;kind++){
     const bool wanted=(position==1?kind==0:kind<4) && (layer%4==3?kind>=2:kind<2);
     std::vector<uint32_t> words={0x13579bdf};
     assert(reference.Snapshot(position+1,layer,kind,words)==wanted);
     if(wanted){const unsigned expected=kind==0?786432:kind==1?30720:(position+1)*512;assert(words.size()==expected);
      const uint32_t magnitude=kind<2?0x7fffffffU:0x7fff7fffU;
      for(auto word:words)assert(!(word&magnitude));records++;words[0]=0x3f800000;std::vector<uint32_t> fresh;
      assert(reference.Snapshot(position+1,layer,kind,fresh) && !(fresh[0]&magnitude));}
     else assert(words==std::vector<uint32_t>{0x13579bdf});
    }
    assert(records==(position==1?48U:128U) && source.reads==reads && reference.Ready());
    printf("PASS CPU-private snapshots/after%u complete steps/%u retained-state records/copied-output mutation cannot affect model/no learned reads;SYNTHETIC ZERO only\n",position+1,records);
   }
   unsigned eosRecords=0;const unsigned eosReads=source.reads;
   for(unsigned layer=0;layer<65;layer++)for(unsigned kind=0;kind<5;kind++){
    const bool wanted=position==1 && layer<64 && kind<4 && (layer%4==3?kind>=2:kind<2);
    std::vector<uint32_t> words={0x13579bdf};
    assert(reference.SnapshotEos(layer,kind,words)==wanted);
    if(wanted){
     const unsigned expected=kind==0?786432:kind==1?30720:1024;
     const uint32_t magnitude=kind<2?0x7fffffffU:0x7fff7fffU;
     assert(words.size()==expected);for(auto word:words)assert(!(word&magnitude));
     words[0]=0x3f800000;std::vector<uint32_t> again;
     assert(reference.SnapshotEos(layer,kind,again) && !(again[0]&magnitude));eosRecords++;
    }else assert(words==std::vector<uint32_t>{0x13579bdf});
   }
   assert(eosRecords==(position==1?128U:0U) && source.reads==eosReads && reference.Ready());
   printf("PASS CPU-private EOS snapshot/after%u steps/%u records/all GDN+Conv+packed K+V at2 ONLY/copied output/invalid geometry and before+after position refuse;SYNTHETIC ZERO only\n",position+1,eosRecords);
   if(position==1){
    // Fork the actual two-step CPU history,not a supplied completed state.
    // A failed POST-copy descriptor audit must not publish any partial copy.
    const pid_t child=fork();assert(child>=0);
    if(!child){
     std::vector<uint32_t> words={0x13579bdf};source.refuseDescription=source.describes+852;
     assert(!reference.SnapshotEos(0,0,words) && words==std::vector<uint32_t>{0x13579bdf} && !reference.Ready());
     source.refuseDescription=0;assert(!reference.SnapshotEos(0,0,words) && !reference.Ready());_exit(0);
    }
    int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0 && reference.Ready());
    puts("PASS CPU-private EOS state/post-copy descriptor loss leaves output unchanged+sticky failed reference;parent continues its SAME third step");
   }
  }
  // A descriptor failure on the POST-copy audit leaves the caller's output
  // untouched and permanently poisons this reference. No partial snapshot.
  std::vector<uint32_t> words={0x13579bdf};source.refuseDescription=source.describes+852;
  assert(!reference.Snapshot(3,0,0,words) && words==std::vector<uint32_t>{0x13579bdf} && !reference.Ready());
  source.refuseDescription=0;assert(!reference.Snapshot(3,0,0,words) && !reference.Ready());
  assert(!reference.SnapshotEos(0,0,words) && words==std::vector<uint32_t>{0x13579bdf} && !reference.Ready());
  puts("PASS independent full CPU reference/3positions/all64+vocab each/tie-lowest/retained state+KV paths/SYNTHETIC ZERO learned bytes only;NOT original model/device/native/Work/TPS proof");
 }
}
