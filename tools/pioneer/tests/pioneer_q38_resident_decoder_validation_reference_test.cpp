/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "pioneer_q38_projection_validation_fixture.h"
#include "../PioneerQ38ResidentDecoderValidationReference.h"
struct DecoderReferenceSource:ProjectionValidationSource {
 std::vector<PioneerQ38Tensor> tensors;unsigned drop=0;
 explicit DecoderReferenceSource(FILE* f):ProjectionValidationSource(f),tensors(model.Tensors()){}
 const auto& Model()const{return *this;}const auto& Tensors()const{return tensors;}
 const PioneerQ38Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  bool okay=ProjectionValidationSource::ReadReferencePacked(tensor,offset,bytes,out,capacity);
  if(referenceReads==drop)valid=false;return okay;
 }
};
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;SeedProjectionValidationEmbedding(fixture,0);
 const char* name="blk.0.ssm_conv1d.weight";std::vector<uint8_t> packed(163840);
 for(unsigned c=0;c<10240;c++)for(unsigned tap=0;tap<4;tap++){
  uint32_t bits=PioneerAttentionBits(float(int((c*7+tap*11)%29)-14)*.03125f);
  for(unsigned b=0;b<4;b++)packed[(c*4+tap)*4+b]=uint8_t(bits>>(8*b));
 }
 for(const auto& t:fixture.expected)if(t.name==name)fixture.Write(t.offset,packed);
 DecoderReferenceSource source(fixture.file);std::vector<uint32_t> input(10240),output(40960,0xdeadbeef);
 for(unsigned c=0;c<10240;c++)input[c]=PioneerAttentionBits(float(int(c%257)-128)*.03125f);
 assert(PioneerQ38ResidentDecoderReference(source,input.data(),10240,output.data(),40960) && source.referenceReads==640);
 for(unsigned c=0;c<10240;c++){
  float sum=0;sum+=PioneerAttentionFloat(input[c])*(float(int((c*7+33)%29)-14)*.03125f); // three zero-history terms precede this add,including signed-zero semantics
  float expected=float(double(sum)/(1+exp(-double(sum))));
  assert(output[c]==PioneerAttentionBits(expected) && !output[10240+c*3] && !output[10240+c*3+1] && output[10240+c*3+2]==input[c]);
 }
 unsigned refusals=0;
 auto refuse=[&](const uint32_t* in,uint32_t* out,unsigned count,unsigned capacity){
  std::fill(output.begin(),output.end(),0xdeadbeef);auto before=input;
  assert(!PioneerQ38ResidentDecoderReference(source,in,count,out,capacity));assert(input==before);
  for(auto v:output)assert(v==0xdeadbeef);refusals++;
 };
 refuse(nullptr,output.data(),10240,40960);refuse(input.data(),nullptr,10240,40960);
 for(unsigned n:{0U,10239U,10241U})refuse(input.data(),output.data(),n,40960);
 for(unsigned n:{0U,40959U,40961U})refuse(input.data(),output.data(),10240,n);
 refuse(input.data(),input.data(),10240,40960);refuse(input.data(),input.data()+1,10240,40960);
 refuse(reinterpret_cast<const uint32_t*>(uintptr_t(1)),output.data(),10240,40960);
 refuse(input.data(),reinterpret_cast<uint32_t*>(uintptr_t(1)),10240,40960);
 refuse(reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3),output.data(),10240,40960);
 refuse(input.data(),reinterpret_cast<uint32_t*>(UINTPTR_MAX-3),10240,40960);
 source.valid=false;refuse(input.data(),output.data(),10240,40960);source.valid=true;
 unsigned tensor=unsigned(source.Find(name)-source.tensors.data());
 for(unsigned field=0;field<6;field++){
  auto saved=source.tensors[tensor];auto& t=source.tensors[tensor];
  if(field==0)t.name="blk.1.ssm_conv1d.weight";if(field==1)t.type=8;if(field==2)t.rank=1;
  if(field==3)t.inputs--;if(field==4)t.outputs--;if(field==5)t.bytes--;
  refuse(input.data(),output.data(),10240,40960);source.tensors[tensor]=saved;
 }
 for(unsigned at:{0U,10239U})for(uint32_t value:{0x7f800000U,0xff800000U,0x7fc00000U}){
  auto saved=input[at];input[at]=value;refuse(input.data(),output.data(),10240,40960);input[at]=saved;
 }
 for(unsigned at:{1U,640U}){
  source.referenceReads=0;source.failReference=at;refuse(input.data(),output.data(),10240,40960);source.failReference=0;
  source.referenceReads=0;source.drop=at;refuse(input.data(),output.data(),10240,40960);source.drop=0;source.valid=true;
 }
 // Even a zero-valued original history must not hide a bad learned tap.
 for(unsigned tap:{0U,3U})for(uint32_t value:{0x7f800000U,0x7fc00000U}){
  std::vector<uint8_t> bad=packed;for(unsigned b=0;b<4;b++)bad[(10239*4+tap)*4+b]=uint8_t(value>>(8*b));
  fixture.Write(source.tensors[tensor].offset,bad);assert(source.model.Open(source.file));source.referenceReads=0;
  refuse(input.data(),output.data(),10240,40960);assert(source.referenceReads==640);
 }fixture.Write(source.tensors[tensor].offset,packed);
 printf("PASS original-F32 first-token Conv4 independent DEBUG oracle ALL10240SiLU/30720exact next values/refusals%u/atomic publication/no device/library access;SYNTHETIC_GGUF_NOT_PHYSICAL_CARD\n",refusals);
}
