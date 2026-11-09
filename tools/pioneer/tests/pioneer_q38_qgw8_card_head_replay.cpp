/* SPDX-License-Identifier: MIT */
// Post-hoc CPU-only observation from one fixed card snapshot, never Work.
#if !defined(PIONEER_Q38_CARD_HEAD_REPLAY) || PIONEER_Q38_CARD_HEAD_REPLAY != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(__FAST_MATH__)
#error Card head replay requires its isolated CPU diagnostic role
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_qgw8_card_hidden_data.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_head_bfp8_cpu_backend.h"
#include "pioneer_q38_distribution_compare.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static bool ReadSnapshot(const char* path,std::vector<uint32_t>& words){
 if(!path || path[0]!='/' || !words.empty())return false;
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat first={},last={};bool good=!fstat(fd,&first) && S_ISREG(first.st_mode)
  && first.st_size==Q38CardHiddenData::Words*4;
 std::vector<uint32_t> data(Q38CardHiddenData::Words);size_t done=0;
 while(good && done<data.size()*4){
  ssize_t n=read(fd,reinterpret_cast<uint8_t*>(data.data())+done,data.size()*4-done);
  if(n<0 && errno==EINTR)continue;if(n<=0){good=false;break;}done+=size_t(n);
 }
 if(fstat(fd,&last) || first.st_dev!=last.st_dev || first.st_ino!=last.st_ino
  || first.st_size!=last.st_size || first.st_mtime!=last.st_mtime || first.st_ctime!=last.st_ctime)good=false;
 if(close(fd))good=false;if(!good)return false;words=std::move(data);return true;
}

static bool Normalize(WholeModelOriginalBytes& source,const std::vector<uint32_t>& raw,std::vector<uint32_t>& output){
 if(raw.size()!=5120 || !source.Ready() || !output.empty())return false;
 unsigned index=UINT32_MAX;
 for(unsigned i=0;i<851;i++){
  Q38OriginalTensor t;if(!source.Describe(i,t))return false;
  if(t.name=="output_norm.weight"){
   if(index!=UINT32_MAX || t.type!=0 || t.rank!=1 || t.inputs!=5120 || t.outputs!=1 || t.bytes!=5120*4)return false;
   index=i;
  }
 }
 if(index==UINT32_MAX)return false;
 std::vector<uint32_t> weights(5120);if(!source.ReadOriginal(index,0,5120*4,weights.data(),5120*4))return false;
 std::vector<float> hidden(5120),norm(5120),normalized;
 for(unsigned i=0;i<5120;i++){hidden[i]=Q38CpuReferenceMath::Float(raw[i]);norm[i]=Q38CpuReferenceMath::Float(weights[i]);}
 if(!Q38CpuReferenceMath::Rms(hidden,norm,normalized) || !source.Ready() || normalized.size()!=5120)return false;
 for(float x:normalized)output.push_back(Q38CpuReferenceMath::Bits(x));return true;
}

int main(int argc,char** argv){
 static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"snapshot byte order");
 if(argc!=6 || strcmp(argv[1],"--card-hidden-head") || argv[2][0]!='/' || argv[3][0]!='/')return 2;
 if(!argv[5][0] || strspn(argv[5],"0123456789")!=strlen(argv[5]) || strlen(argv[5])>6)return 2;
 unsigned long observed=strtoul(argv[5],nullptr,10);if(observed>=248320)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 std::vector<uint32_t> words,hidden;
 if(!ReadSnapshot(argv[3],words) || !Q38CardHiddenData::Decode(words,argv[4],hidden)){
  fputs("CARD_HIDDEN_INVALID; NO MODEL OR DEVICE\n",stderr);return 1;
 }
 puts("CARD_HEAD_REPLAY authenticating original model; CPU_ONLY_NO_DEVICE_OR_WORK; INPUT_PROVENANCE_NOT_PROVEN");
 WholeModelOriginalBytes source;if(!source.Open(argv[2]) || !source.Ready())return 1;
 std::vector<uint32_t> normalized,logits;
 if(!Normalize(source,hidden,normalized) || !Q38HeadBfp8CpuObserve(argv[2],normalized,logits)
  || logits.size()!=248320 || !source.Ready())return 1;
 Q38DistributionCompare::ObservedToken r={},reference={};
 // This snapshot format fixes input 248319 and position zero. The saved,
 // independently pinned original-Q4 CPU trace selected token 70253 there.
 // Comparing both ranks uses the SAME replayed hidden vector and logits;
 // it is post-hoc diagnostic data, not validation performed during Work.
 if(!Q38DistributionCompare::ObservePair(logits,unsigned(observed),70253,r,reference))return 1;
 printf("CARD_HEAD_REPLAY observed_token=%u software_top=%u observed_rank=%u observed_logit=%.9g top_logit=%.9g probability=%.9g; CPU_ONLY_NOT_QUALITY_OR_SPEED; INPUT_PROVENANCE_NOT_PROVEN\n",
  r.token,r.top,r.rank,r.logit,r.topLogit,r.probability);
 printf("CARD_HEAD_REFERENCE original_cpu_token=%u rank=%u logit=%.9g probability=%.9g; SAME_REPLAYED_CARD_HIDDEN; CPU_ONLY_NOT_QUALITY_OR_SPEED; INPUT_PROVENANCE_NOT_PROVEN\n",
  reference.token,reference.rank,reference.logit,reference.probability);
 return fflush(stdout)?1:0;
}
