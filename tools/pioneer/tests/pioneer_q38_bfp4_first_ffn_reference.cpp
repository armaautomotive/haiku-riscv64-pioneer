/* SPDX-License-Identifier: MIT */
// CPU-only first-FFN trace. Private data; never a device input or Work mode.
#define main PioneerPriorPromptInputMain
#include "pioneer_q38_bfp4_prompt_input.cpp"
#undef main
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--first-ffn-reference"))return 2;
 const char* directory="/boot/home/pioneer-bfp4-ffn-quality-20261106-a";
 const char* output="/boot/home/pioneer-bfp4-ffn-quality-20261106-a/reference.bin";
 struct stat info={};if(lstat(directory,&info) || !S_ISDIR(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&07777)!=0700)return 1;
 const uint32_t endian=1;if(*reinterpret_cast<const uint8_t*>(&endian)!=1)return 1;
 puts("FFN_REFERENCE: authenticating original; CPU-only, no device");if(fflush(stdout))return 1;
 AuthenticatedRows source;if(!source.Open())return 1;
 Q38CpuModelReference observer;Q38CpuFirstFfnTrace trace;
 if(!observer.Bind(source) || !observer.ObserveFirstFfnTrace(248319,trace) || observer.Ready() || !source.Ready())return 1;
 if(trace.input.size()!=5120 || trace.residual.size()!=5120 || trace.gate.size()!=17408 || trace.up.size()!=17408
  || trace.activation.size()!=17408 || trace.down.size()!=5120 || trace.final.size()!=5120)return 1;
 if(PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(trace.final.data(),trace.final.size()*4))!="0fc71cc5a79d621c65b45aceb6bf705b1989d2b2385af99c8e2a2fbb5e0c064c"){
  fputs("FFN_REFERENCE_REFUSED: output does not match pinned trace; no data published\n",stderr);return 1;
 }
 std::vector<uint32_t> data={0x31464651,1,248319,5120,17408,72704,0,0};
 for(const auto* field:{&trace.input,&trace.residual,&trace.gate,&trace.up,&trace.activation,&trace.down,&trace.final})
  data.insert(data.end(),field->begin(),field->end());
 if(data.size()!=72712 || !source.Ready())return 1;
 const auto digest=PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(data.data(),data.size()*4));
 int fd=open(output,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0)return 1;
 const auto* bytes=reinterpret_cast<const uint8_t*>(data.data());size_t done=0,total=data.size()*4;
 while(done<total){ssize_t n=write(fd,bytes+done,total-done);if(n<=0){close(fd);return 1;}done+=size_t(n);}
 const bool synced=!fsync(fd),closed=!close(fd);if(!synced || !closed || !source.Ready())return 1;
 printf("PASS_FFN_REFERENCE token248319 layer0 words72704 pinned_final_hidden_exact1 sha256%s; CPU_ONLY_NO_DEVICE\n",digest.c_str());
 return fflush(stdout)?1:0;
}
