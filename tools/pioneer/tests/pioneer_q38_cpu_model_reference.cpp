/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
struct Q38CpuModelReference::Impl {
 struct Shape{const char* name;unsigned inputs,outputs,type,rank;};
 struct LayerState{std::vector<float> history,state,key,value;};
 Q38OriginalRows* source=nullptr;uint64_t epoch=0;unsigned position=0;
 bool attempted=false,bound=false,failed=false;
 std::array<Q38OriginalTensor,851> tensors;
 std::array<std::array<unsigned,14>,65> index={};
 std::array<LayerState,64> state;
 static bool Expected(unsigned layer,unsigned role,Q38OriginalTensor& out){
  static const Shape common[]={
   {"attn_norm.weight",5120,1,0,1},{"post_attention_norm.weight",5120,1,0,1},
   {"ffn_gate.weight",5120,17408,12,2},{"ffn_up.weight",5120,17408,12,2},{"ffn_down.weight",17408,5120,12,2}};
  static const Shape delta[]={
   {"attn_gate.weight",5120,6144,8,2},{"attn_qkv.weight",5120,10240,8,2},{"ssm_a",48,1,0,1},
   {"ssm_dt.bias",48,1,0,1},{"ssm_norm.weight",128,1,0,1},{"ssm_alpha.weight",5120,48,8,2},
   {"ssm_beta.weight",5120,48,8,2},{"ssm_conv1d.weight",4,10240,0,2},{"ssm_out.weight",6144,5120,8,2}};
  static const Shape attention[]={
   {"attn_q.weight",5120,12288,8,2},{"attn_k.weight",5120,1024,8,2},{"attn_v.weight",5120,1024,8,2},
   {"attn_q_norm.weight",256,1,0,1},{"attn_k_norm.weight",256,1,0,1},{"attn_output.weight",6144,5120,14,2}};
  static const Shape global[]={
   {"token_embd.weight",5120,248320,12,2},{"output.weight",5120,248320,14,2},{"output_norm.weight",5120,1,0,1}};
  const Shape* s=nullptr;
  if(layer==64){if(role>=3)return false;s=global+role;out.name=s->name;}
  else{
   if(layer>=64 || role>=(layer%4==3?11U:14U))return false;
   s=role<5?common+role:layer%4==3?attention+(role-5):delta+(role-5);
   out.name="blk."+std::to_string(layer)+"."+s->name;
  }
  unsigned width=s->type==8?32:s->type?256:1,packed=s->type==8?34:s->type==12?144:s->type==14?210:4;
  out.type=s->type;out.rank=s->rank;out.inputs=s->inputs;out.outputs=s->outputs;out.bytes=uint64_t(s->inputs/width)*packed*s->outputs;
  return true;
 }
 bool Current(bool descriptors=false)const{
  if(!bound || failed || !source || !source->Ready() || !epoch || source->Epoch()!=epoch || std::fegetround()!=FE_TONEAREST)return false;
  if(descriptors)for(unsigned i=0;i<851;i++){Q38OriginalTensor now;if(!source->Describe(i,now) || !(now==tensors[i]))return false;}
  return source->Ready() && source->Epoch()==epoch;
 }
 bool Bind(Q38OriginalRows& s){
  if(attempted)return false;attempted=true;
  if(!s.Ready() || !s.Epoch() || std::fegetround()!=FE_TONEAREST)return false;
  source=&s;epoch=s.Epoch();std::map<std::string,unsigned> names;
  for(unsigned i=0;i<851;i++)if(!s.Describe(i,tensors[i]) || !names.emplace(tensors[i].name,i).second)return false;
  Q38OriginalTensor extra;if(s.Describe(851,extra))return false;unsigned matched=0;
  for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;role<14;role++){
   Q38OriginalTensor expected;if(!Expected(layer,role,expected))break;
   auto found=names.find(expected.name);if(found==names.end())return false;unsigned i=found->second;
   const auto& t=tensors[i];expected.offset=t.offset;
   if(!(t==expected))return false;index[layer][role]=i;matched++;
  }
  if(matched!=851)return false;bound=true;return Current(true);
 }
 bool Read(unsigned tensor,uint64_t at,unsigned bytes,void* output){
  if(tensor>=851 || !Current())return false;
  Q38OriginalTensor now;if(!source->Describe(tensor,now) || !(now==tensors[tensor]))return false;
  return source->ReadOriginal(tensor,at,bytes,output,bytes) && Current();
 }
 bool Weight(unsigned layer,unsigned role,std::vector<float>& result){
  unsigned i=index[layer][role];const auto& t=tensors[i];if(t.type || t.bytes>1048576 || t.bytes%4)return false;
  std::vector<uint8_t> bytes(t.bytes);if(!Read(i,0,unsigned(bytes.size()),bytes.data()))return false;
  std::vector<float> out(bytes.size()/4);
  for(unsigned n=0;n<out.size();n++){const auto* p=bytes.data()+n*4;out[n]=Float(uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24);if(!Bounded(out[n]))return false;}
  result=std::move(out);return true;
 }
 bool Project(unsigned layer,unsigned role,const std::vector<float>& input,std::vector<float>& output){
  unsigned i=index[layer][role];const auto& t=tensors[i];
  bool okay=Matrix(t.type,unsigned(t.inputs),unsigned(t.outputs),input,output,[&](uint64_t at,unsigned bytes,void* out){return Read(i,at,bytes,out);});
  if(!okay)fprintf(stderr,"CPU_REFERENCE_MATRIX_REFUSAL position%u/layer%u/role%u/type%u;NO trace publication or retry\n",position,layer,role,t.type);
  return okay;
 }
 bool Embedding(unsigned token,std::vector<float>& output){
  unsigned i=index[64][0];std::array<uint8_t,2880> bytes;if(token>=248320 || !Read(i,uint64_t(token)*2880,2880,bytes.data()))return false;
  std::vector<float> out(5120);
  for(unsigned block=0;block<20;block++){
   const auto* p=bytes.data()+block*144;float d=Half(U16(p)),m=Half(U16(p+2));if(!std::isfinite(d) || !std::isfinite(m))return false;
   for(unsigned n=0;n<256;n++){out[block*256+n]=(d*float(Scale4(p,n/32)))*float(Quant(12,p,n))-(m*float(Min4(p,n/32)));if(!Bounded(out[block*256+n]))return false;}
  }
  output=std::move(out);return true;
 }
 bool Recurrent(unsigned layer,const std::vector<float>& normalized,std::vector<float>& output){
  std::vector<float> z,qkv,alpha,beta,convWeight,a,dt,gatedNorm;
  if(!Project(layer,5,normalized,z) || !Project(layer,6,normalized,qkv) || !Project(layer,10,normalized,alpha) || !Project(layer,11,normalized,beta)
   || !Weight(layer,12,convWeight) || !Weight(layer,7,a) || !Weight(layer,8,dt) || !Weight(layer,9,gatedNorm))return false;
  auto& s=state[layer];
  if(!position){s.history.assign(30720,0);s.state.assign(786432,0);}
  if(s.history.size()!=30720 || s.state.size()!=786432)return false;
  std::vector<float> conv(10240),q(2048),k(2048),raw(6144),gated(6144);
  for(unsigned c=0;c<10240;c++){
   float sum=0;for(unsigned tap=0;tap<4;tap++)sum+=(tap==3?qkv[c]:s.history[c*3+tap])*convWeight[c*4+tap];
   double x=sum;conv[c]=float(x*SigmoidDouble(sum));if(!Normal(conv[c]))return false;
   s.history[c*3]=s.history[c*3+1];s.history[c*3+1]=s.history[c*3+2];s.history[c*3+2]=qkv[c];
  }
  for(unsigned h=0;h<16;h++){
   float qs=0,ks=0;for(unsigned i=0;i<128;i++){float x=conv[h*128+i],y=conv[2048+h*128+i];qs+=x*x;ks+=y*y;}
   float qi=1.f/std::sqrt(qs+1e-6f),ki=1.f/std::sqrt(ks+1e-6f);if(!Normal(qi) || !Normal(ki))return false;
   for(unsigned i=0;i<128;i++){q[h*128+i]=conv[h*128+i]*qi;k[h*128+i]=conv[2048+h*128+i]*ki;}
  }
  for(unsigned h=0;h<48;h++){
   if(!(a[h]<0))return false;double shifted=alpha[h]+dt[h];
   float softplus=float(shifted>0?shifted+std::log1p(std::exp(-shifted)):std::log1p(std::exp(shifted)));
   float decay=Flush(float(std::exp(double(a[h]*softplus)))),gate=Flush(Sigmoid(beta[h]));
   if(!Gdn(q.data()+(h%16)*128,k.data()+(h%16)*128,conv.data()+4096+h*128,decay,gate,s.state.data()+h*16384,raw.data()+h*128))return false;
   float sum=0;for(unsigned i=0;i<128;i++){float x=raw[h*128+i];sum+=Flush(x*x);}
   float inverse=1.f/std::sqrt(Flush(sum*.0078125f)+1e-6f);if(!Normal(inverse) || !(inverse>0))return false;
   for(unsigned i=0;i<128;i++){
    unsigned at=h*128+i;float weighted=Flush(Flush(raw[at]*inverse)*gatedNorm[i]);
    gated[at]=Flush(float(double(weighted)*double(z[at])*SigmoidDouble(z[at])));if(!Normal(gated[at]))return false;
   }
  }
  return Project(layer,13,gated,output);
 }
 bool Dense(unsigned layer,const std::vector<float>& normalized,std::vector<float>& output){
  std::vector<float> qg,key,value,qnorm,knorm,attention;
  if(!Project(layer,5,normalized,qg) || !Project(layer,6,normalized,key) || !Project(layer,7,normalized,value)
   || !Weight(layer,8,qnorm) || !Weight(layer,9,knorm))return false;
  auto& s=state[layer];
  return Attention(qg,key,value,qnorm,knorm,position,s.key,s.value,attention) && Project(layer,10,attention,output);
 }
 bool Step(unsigned token,Q38CpuTokenTrace& output){
  if(token>=248320 || position>=8192 || !Current(true))return false;
  Q38CpuTokenTrace stage;stage.position=position;stage.input=token;stage.hidden.reserve(65*5120);
  std::vector<float> hidden;if(!Embedding(token,hidden))return false;
  auto record=[&](){for(float x:hidden)stage.hidden.push_back(Bits(x));};record();
  for(unsigned layer=0;layer<64;layer++){
   if(!Current(true))return false;
   printf("CPU_REFERENCE_BEGIN position%u/layer%u/independent ORIGINAL row-major forward\n",position,layer);
   std::vector<float> norm,post,normalized,projection,residual(5120),postNormalized,gate,up,activation(17408),down;
   if(!Weight(layer,0,norm) || !Weight(layer,1,post) || !Rms(hidden,norm,normalized))return false;
   if(!(layer%4==3?Dense(layer,normalized,projection):Recurrent(layer,normalized,projection)))return false;
   for(unsigned i=0;i<5120;i++){residual[i]=hidden[i]+projection[i];if(!Bounded(residual[i],0x4c800000))return false;}
   if(!Rms(residual,post,postNormalized) || !Project(layer,2,postNormalized,gate) || !Project(layer,3,postNormalized,up))return false;
   for(unsigned i=0;i<17408;i++){if(!Bounded(gate[i]) || !Bounded(up[i]))return false;activation[i]=(gate[i]*Sigmoid(gate[i]))*up[i];if(!Bounded(activation[i]))return false;}
   if(!Project(layer,4,activation,down))return false;
   for(unsigned i=0;i<5120;i++){hidden[i]=residual[i]+down[i];if(!Bounded(hidden[i],0x4c800000))return false;}
   if(!Current(true))return false;record();
   printf("CPU_REFERENCE position%u/layer%u/original row-major weights/independent retained state;CPU-private only\n",position,layer);
  }
  std::vector<float> norm,normalized,logits;if(!Weight(64,2,norm) || !Rms(hidden,norm,normalized) || !Project(64,1,normalized,logits) || logits.size()!=248320)return false;
  unsigned best=0,second=1;if(logits[second]>logits[best])std::swap(best,second);
  for(unsigned i=2;i<logits.size();i++)if(logits[i]>logits[best]){second=best;best=i;}else if(logits[i]>logits[second])second=i;
  stage.selected=best;stage.runnerUp=second;stage.best=Bits(logits[best]);stage.second=Bits(logits[second]);stage.logits.reserve(logits.size());
  for(float x:logits){if(!Normal(x))return false;stage.logits.push_back(Bits(x));}
  if(!Current(true))return false;output=std::move(stage);position++;return true;
 }
 bool Snapshot(unsigned completed,unsigned layer,unsigned kind,std::vector<uint32_t>& output,bool eos=false){
  if(completed!=position || (completed!=2 && completed!=3) || layer>=64 || kind>3
   || (eos?completed!=2:completed==2 && kind!=0) || (layer%4==3?kind<2:kind>=2))return false;
  if(!Current(true)){failed=true;return false;}
  const auto& s=state[layer];const auto& values=kind==0?s.state:kind==1?s.history:kind==2?s.key:s.value;
  const unsigned expected=kind==0?786432:kind==1?30720:completed*1024;
  if(values.size()!=expected){failed=true;return false;}
  std::vector<uint32_t> stage;stage.reserve(kind<2?expected:expected/2);
  if(kind<2){for(float value:values){if(!Normal(value)){failed=true;return false;}stage.push_back(Bits(value));}}
  else for(unsigned i=0;i<expected;i+=2){
   if(!Normal(values[i]) || !Normal(values[i+1])){failed=true;return false;}
   const uint16_t low=HalfRne(values[i]),high=HalfRne(values[i+1]);
   // Attention already half-rounds its retained cache. Refuse an API that
   // would silently quantize a different FP32 history for the comparison.
   if(Bits(Half(low))!=Bits(values[i]) || Bits(Half(high))!=Bits(values[i+1])){failed=true;return false;}
   stage.push_back(uint32_t(low)|uint32_t(high)<<16);
  }
  if(!Current(true)){failed=true;return false;}output=std::move(stage);return true;
 }
};
Q38CpuModelReference::Q38CpuModelReference():impl(new Impl){}
Q38CpuModelReference::~Q38CpuModelReference()=default;
bool Q38CpuModelReference::Bind(Q38OriginalRows& s){return impl->Bind(s);}
bool Q38CpuModelReference::Ready()const{return impl->Current(true);}
bool Q38CpuModelReference::Step(unsigned token,Q38CpuTokenTrace& out){
 if(!impl->Step(token,out)){impl->failed=true;return false;}return true;
}
bool Q38CpuModelReference::Snapshot(unsigned completed,unsigned layer,unsigned kind,std::vector<uint32_t>& out){return impl->Snapshot(completed,layer,kind,out);}
bool Q38CpuModelReference::SnapshotEos(unsigned layer,unsigned kind,std::vector<uint32_t>& out){return impl->Snapshot(2,layer,kind,out,true);}
