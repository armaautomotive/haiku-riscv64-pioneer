/* SPDX-License-Identifier: MIT */
// Offline CPU validation values for the exact synthetic BF16 input used by
// the BFP8 bridge. Never linked into the device or no-reference Work mode.
#define main recurrent_learned_export_main
#include "pioneer_q38_recurrent_learned_export.cpp"
#undef main
#define PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_REFERENCE 1
#include "../PioneerQ38RecurrentBlockValidationReference.h"

int main(int argc,char** argv){
 if(argc!=3 || argv[1][0]!='/' || argv[2][0]!='/')return 2;
 modelPath=argv[1];PioneerQ38PublicModelSource source;assert(source.Open() && source.Ready());
 PioneerRecurrentReferenceDetail::View<PioneerQ38PublicModelSource> view(source);assert(view.Bind());
 std::array<uint32_t,5120> input{};std::array<uint32_t,96> result{};
 for(unsigned i=0;i<input.size();i++)input[i]=uint32_t(uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0)))<<16;
 assert(view.Matrix(3,input.data(),result.data()) && view.Matrix(4,input.data(),result.data()+48) && view.Current());
 int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);assert(fd>=0);
 const uint8_t* bytes=reinterpret_cast<const uint8_t*>(result.data());size_t at=0;
 while(at<sizeof(result)){ssize_t n=write(fd,bytes+at,sizeof(result)-at);assert(n>0);at+=size_t(n);}
 assert(!fsync(fd) && !close(fd) && source.Ready());
 std::printf("PASS offline Q8 alpha/beta reference, authenticated weights and SAME synthetic BF16 input; NO DEVICE OR SPEED\n");
}
