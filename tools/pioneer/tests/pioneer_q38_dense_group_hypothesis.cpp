/* SPDX-License-Identifier: MIT */
// CPU-only hypothesis screen. This is not a device implementation or test.
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <array>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <map>
#include <sys/stat.h>
#include <unistd.h>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"

static bool ReadSlot(const char* path,unsigned slot,std::vector<float>& data){
 FILE* f=fopen(path,"rb");if(!f)return false;
 std::vector<float> tmp(5120);
 bool okay=fseek(f,long(slot)*5120*4,SEEK_SET)==0 && fread(tmp.data(),4,tmp.size(),f)==tmp.size();
 if(fclose(f))okay=false;
 for(float x:tmp)if(!std::isfinite(x))okay=false;
 if(okay)data=std::move(tmp);
 return okay;
}
static bool Compare(const char* name,const std::vector<float>& reference,const std::vector<float>& actual){
 double error=0,den=0,dot=0,norm=0;
 for(unsigned i=0;i<5120;i++){
  double a=actual[i],r=reference[i],d=a-r;error+=d*d;den+=r*r;dot+=a*r;norm+=a*a;
 }
 if(!(den>0) || !(norm>0) || !std::isfinite(error) || !std::isfinite(dot))return false;
 printf("GROUP_HYPOTHESIS %s rel_l2_to_card=%.9g cosine=%.9g; CPU_ONLY\n",name,std::sqrt(error/den),dot/std::sqrt(den*norm));
 return true;
}
static bool Save(const char* path,const std::vector<float>& values){
 if(!path || path[0]!='/' || values.size()!=6144)return false;
 for(float v:values)if(!std::isfinite(v))return false;
 int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0)return false;
 struct stat info={};bool okay=!fstat(fd,&info) && S_ISREG(info.st_mode) && info.st_nlink==1
  && info.st_uid==geteuid() && !(info.st_mode&077);
 const auto* data=reinterpret_cast<const unsigned char*>(values.data());size_t left=values.size()*sizeof(float);
 while(okay && left){ssize_t n=write(fd,data,left);if(n<0 && errno==EINTR)continue;
  if(n<=0){okay=false;break;}data+=n;left-=size_t(n);}
 if(okay && fsync(fd))okay=false;
 if(close(fd))okay=false;
 return okay && !left;
}
int main(int argc,char** argv){
 static_assert(sizeof(float)==4 && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"little-endian float required");
 if((argc!=5 && argc!=7) || (argc==7 && (strcmp(argv[5],"--save-reference") || argv[6][0]!='/'))
  || std::fegetround()!=FE_TONEAREST)return 2;
 std::vector<float> input,card,cpu,norm,normalized,qg,value;
 if(!ReadSlot(argv[2],2,input) || !ReadSlot(argv[3],0,card) || !ReadSlot(argv[4],0,cpu))return 1;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(model) || !replay.Weight(3,0,norm) || !Rms(input,norm,normalized)
  || !replay.Project(3,5,normalized,qg) || !replay.Project(3,7,normalized,value)
  || qg.size()!=12288 || value.size()!=1024)return 1;
 for(unsigned mode=0;mode<4;mode++){
  std::vector<float> attention(6144),projection;
  for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){
   unsigned group=mode==0?h/6:mode==1?h%4:mode==2?3-h/6:0;
   float v=Half(HalfRne(value[group*256+i]));
   attention[h*256+i]=Flush(v*Sigmoid(qg[h*512+256+i]));
  }
  if(!replay.Project(3,10,attention,projection) || projection.size()!=5120)return 1;
  if(mode==0 && memcmp(projection.data(),cpu.data(),5120*sizeof(float))){
   fputs("GROUP_HYPOTHESIS_REFUSED normal CPU projection did not reproduce pinned reference\n",stderr);return 1;
  }
  if(mode==0 && argc==7 && !Save(argv[6],attention)){
   fputs("GROUP_HYPOTHESIS_REFUSED reference save failed; partial file is untrusted\n",stderr);return 1;
  }
  const char* names[]={"normal","interleaved","reverse","all_group_zero"};
  if(!Compare(names[mode],card,projection))return 1;
 }
 // Each group occupies six contiguous heads, a whole number of Q6 input
 // quantization chunks. Fit four CPU-derived group contributions to the
 // observed card projection. This is a hypothesis screen, not device math.
 std::array<std::vector<float>,4> basis;
 for(unsigned group=0;group<4;group++){
  std::vector<float> attention(6144,0);
  for(unsigned h=group*6;h<(group+1)*6;h++)for(unsigned i=0;i<256;i++)
   attention[h*256+i]=Flush(Half(HalfRne(value[group*256+i]))*Sigmoid(qg[h*512+256+i]));
  if(!replay.Project(3,10,attention,basis[group]) || basis[group].size()!=5120)return 1;
 }
 double splitError=0,splitDen=0;
 for(unsigned n=0;n<5120;n++){
  double sum=0;for(unsigned group=0;group<4;group++)sum+=basis[group][n];
  double d=sum-cpu[n];splitError+=d*d;splitDen+=double(cpu[n])*cpu[n];
 }
 if(!(splitDen>0) || std::sqrt(splitError/splitDen)>1e-5){
  fputs("GROUP_FIT_REFUSED group decomposition does not reproduce CPU projection\n",stderr);return 1;
 }
 double matrix[4][5]={};
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<5;j++)for(unsigned n=0;n<5120;n++)
  matrix[i][j]+=double(basis[i][n])*(j<4?basis[j][n]:card[n]);
 for(unsigned i=0;i<4;i++){
  unsigned pivot=i;for(unsigned j=i+1;j<4;j++)if(std::fabs(matrix[j][i])>std::fabs(matrix[pivot][i]))pivot=j;
  if(!(std::fabs(matrix[pivot][i])>1e-20))return 1;
  for(unsigned j=0;j<5;j++)std::swap(matrix[i][j],matrix[pivot][j]);
  const double scale=matrix[i][i];for(unsigned j=i;j<5;j++)matrix[i][j]/=scale;
  for(unsigned row=0;row<4;row++)if(row!=i){
   const double factor=matrix[row][i];for(unsigned col=i;col<5;col++)matrix[row][col]-=factor*matrix[i][col];
  }
 }
 double error=0,den=0;for(unsigned n=0;n<5120;n++){
  double fitted=0;for(unsigned group=0;group<4;group++)fitted+=matrix[group][4]*basis[group][n];
  double d=fitted-card[n];error+=d*d;den+=double(card[n])*card[n];
 }
 if(!(den>0) || !std::isfinite(error))return 1;
 printf("GROUP_LINEAR_FIT coefficients=%.9g,%.9g,%.9g,%.9g rel_l2_to_card=%.9g decomposition_rel_l2=%.9g; CPU_ONLY HYPOTHESIS NOT_CAUSE\n",
  matrix[0][4],matrix[1][4],matrix[2][4],matrix[3][4],std::sqrt(error/den),std::sqrt(splitError/splitDen));
 if(!model.Ready())return 1;
 puts("CPU_ONLY_HYPOTHESES; not device provenance, proof of cause, or performance");
}
