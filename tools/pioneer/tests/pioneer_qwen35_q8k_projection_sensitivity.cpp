/* SPDX-License-Identifier: MIT */
// Offline-only examination of a position-zero projection sensitivity probe.
// The two input rows and model must be from the separately authenticated oracle
// experiment. This program is not part of Work mode and never uses the card.
#include "ggml.h"
#include "ggml-cpu.h"
#include "gguf.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
int main(int argc,char**argv){
 if(argc!=4 && argc!=5){std::fprintf(stderr,"usage: %s model.gguf control-f32.bin perturbed-f32.bin [tensor-name]\n",argv[0]);return 2;}std::array<std::vector<float>,2> in{};
 for(int i=0;i<2;i++){
  FILE*f=std::fopen(argv[i+2],"rb");if(!f)return 1;
  if(fseeko(f,0,SEEK_END))return 1;const off_t size=ftello(f);
  if(size<=0 || size>9216*4 || size%(256*4) || fseeko(f,0,SEEK_SET))return 1;
  in[i].resize(size_t(size)/4);bool ok=std::fread(in[i].data(),4,in[i].size(),f)==in[i].size() && std::fgetc(f)==EOF;
  if(std::fclose(f)||!ok)return 1;
 }
 if(in[0].size()!=in[1].size())return 1;const unsigned width=unsigned(in[0].size());
 for(const auto& row:in)for(float value:row)if(!std::isfinite(value))return 1;
 auto*q=ggml_get_type_traits_cpu(GGML_TYPE_Q8_K);size_t nb=ggml_row_size(GGML_TYPE_Q8_K,width);if(!q||!q->from_float||nb!=size_t(width/256)*292)return 1;
 std::vector<unsigned char>a(nb),b(nb),scalesOnly(nb);q->from_float(in[0].data(),a.data(),width);q->from_float(in[1].data(),b.data(),width);scalesOnly=b;
 unsigned scaleBlocks=0,quantValues=0,blockSums=0;
 for(unsigned block=0;block<width/256;block++){
  const unsigned offset=block*292;
  scaleBlocks+=std::memcmp(a.data()+offset,b.data()+offset,4)!=0;
  for(unsigned j=4;j<260;j++)quantValues+=a[offset+j]!=b[offset+j];
  for(unsigned j=260;j<292;j+=2)blockSums+=std::memcmp(a.data()+offset+j,b.data()+offset+j,2)!=0;
 }
 std::printf("Q8_K: %u differing block scales, %u quantized values, %u block sums\n",scaleBlocks,quantValues,blockSums);
 for(unsigned block=0;block<width/256;block++)std::memcpy(scalesOnly.data()+block*292+4,a.data()+block*292+4,288);
 gguf_init_params p{true,nullptr};auto*c=gguf_init_from_file(argv[1],p);if(!c)return 1;FILE*f=std::fopen(argv[1],"rb");if(!f)return 1;unsigned found=0;
 for(int64_t i=0;i<gguf_get_n_tensors(c);i++){
  const char*n=gguf_get_tensor_name(c,i);if(argc==5?std::strcmp(n,argv[4]):(std::strcmp(n,"blk.25.attn_qkv.weight")&&std::strcmp(n,"blk.25.attn_gate.weight")))continue;
  auto type=gguf_get_tensor_type(c,i);auto*t=ggml_get_type_traits_cpu(type);auto dims=gguf_get_tensor_ne(c,i);if(dims[0]!=width||dims[1]<=0||dims[1]>10000||!t||!t->vec_dot||t->vec_dot_type!=GGML_TYPE_Q8_K)return 1;
  size_t row=ggml_row_size(type,width);std::vector<unsigned char>w(row);if(fseeko(f,off_t(gguf_get_data_offset(c)+gguf_get_tensor_offset(c,i)),SEEK_SET))return 1;
  double square=0,squareScale=0;float max=0,maxScale=0;
  for(int64_t k=0;k<dims[1];k++){if(std::fread(w.data(),1,row,f)!=row)return 1;float x=0,y=0,z=0;t->vec_dot(width,&x,0,w.data(),0,a.data(),0,1);t->vec_dot(width,&y,0,w.data(),0,b.data(),0,1);t->vec_dot(width,&z,0,w.data(),0,scalesOnly.data(),0,1);float d=std::fabs(y-x),ds=std::fabs(z-x);if(d>max)max=d;if(ds>maxScale)maxScale=ds;square+=d*d;squareScale+=ds*ds;}
  std::printf("%s %s rows %lld: full max %.9g rms %.9g; scales-only max %.9g rms %.9g\n",n,ggml_type_name(type),(long long)dims[1],max,std::sqrt(square/dims[1]),maxScale,std::sqrt(squareScale/dims[1]));found++;
 }gguf_free(c);std::fclose(f);return found==unsigned(argc==5?1:2)?0:1;
}
