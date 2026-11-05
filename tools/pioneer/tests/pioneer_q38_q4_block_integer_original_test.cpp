/* SPDX-License-Identifier: MIT */
// Original-weight arithmetic samples with synthetic activations,not inference.
#define PIONEER_Q38_Q4_BLOCK_INTEGER_AFFINE_TEST_MODEL_ONLY
#include "pioneer_q38_q4_block_integer_affine_test.cpp"
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
#if PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST != 1
#error Separate emitted-state original-sample test only
#endif
#define PIONEER_Q38_Q4_BLOCK_INTEGER_STATE_MACHINE_ONLY
#include "pioneer_q38_q4_block_integer_state_test.cpp"
#endif
#include "../PioneerModelWeightCache.h"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerQwen38Panel.h"
#include <fcntl.h>
#include <unistd.h>
static bool Same(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/' && SM(-1)==0x80000001);setvbuf(stdout,nullptr,_IOLBF,0);
 const int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));
 PioneerModelWeightCache::Digest digest;std::string error;
 puts("BLOCK_INTEGER_AUTH_ORIGINAL_SAME_OPEN_READ_ONLY_NO_CACHE_OR_DEVICE");
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  && PioneerModelWeightCache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto model=std::make_unique<PioneerQwen38Gguf>();assert(model->Open(file) && model->Tensors().size()==851);
 AffineWords panels[2],finishes[2];IntegerAffine::Panel<0>(panels[0]);IntegerAffine::Panel<2>(panels[1]);
 IntegerAffine::Finish<0>(finishes[0]);IntegerAffine::Finish<2>(finishes[1]);
 unsigned tensors=0,jobs=0,words=0,changed=0;uint32_t seed=0x27b25608;
 auto random=[&](){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 for(unsigned tensor=0;tensor<model->Tensors().size();tensor++){
  const auto& entry=model->Tensors()[tensor];if(entry.type!=12)continue;
  assert(entry.rank==2 && entry.outputs%16==0 && entry.inputs%256==0);tensors++;
  const unsigned blocks=unsigned(entry.inputs/256),rowBytes=blocks*144;
  for(unsigned first:{0U,unsigned(entry.outputs/32)*16,unsigned(entry.outputs)-16})for(unsigned block:{0U,blocks/2,blocks-1}){
   uint8_t rows[16][144],tile[2304];
   for(unsigned row=0;row<16;row++){
    assert(model->ReadPacked(tensor,uint64_t(first+row)*rowBytes+block*144,144,rows[row],144));
    for(unsigned byte=0;byte<144;byte++)tile[byte*16+row]=rows[row][byte];
   }
   PioneerQ38Panel decoded[16];for(unsigned p=0;p<16;p++)assert(PioneerQ38UnpackPanel(12,tile,2304,p,&decoded[p]));
   for(unsigned job=0;job<2;job++){
    IntegerVector m;std::array<std::array<int64_t,32>,2> dot{},minimum{},saved{};
    std::array<std::array<float,32>,2> legacy{};std::array<std::array<uint32_t,32>,2> second{},savedMin{};
    std::array<int,256> input;for(auto& x:input)x=int(random()%256)-128;
    const float activation=Float(job?0x3b812345:0x3f800000);
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
    const unsigned bases[]={0x90000,0x98000,0x98900,0x9a000,0x9a900};
    auto stateMachine=std::make_unique<IntegerStateMachine>(bases[(tensor+first+block+job)%5]);State state;
    assert(state.Init(*stateMachine));memcpy(stateMachine->tile.data(),tile,2304);
    assert(state.Begin(*stateMachine,stateMachine->base));stateMachine->Closed();assert(!MetadataCheck(*stateMachine));
    stateMachine->Config(1,0x60aa0000);
#endif
    for(unsigned p=0;p<16;p++){
     if(p==8)for(unsigned parity=0;parity<2;parity++)for(unsigned lane=0;lane<32;lane++){
      saved[parity][lane]=dot[parity][lane];dot[parity][lane]=0;m.lr[4+parity*2][lane]=0;
     }
     for(unsigned parity=0;parity<2;parity++){
      for(unsigned lane=0;lane<32;lane++){
       const unsigned row=(lane%8)*2+parity,g=p/2;const auto* raw=rows[row];
       const unsigned factor=g<4?raw[4+g]&63:(raw[8+g]&15)|((raw[g]>>6)<<4);
       const unsigned min=g<4?raw[8+g]&63:(raw[8+g]>>4)|((raw[4+g]>>6)<<4);
       const float d=Half(raw[0]|unsigned(raw[1])<<8),dm=Half(raw[2]|unsigned(raw[3])<<8);
       assert(Bits(d*float(factor))==decoded[p].coefficient[row] && Bits(dm*float(-int(min)))==decoded[p].minimum[row]);
       int qdot=0,sum=0;
       for(unsigned k=0;k<16;k++){
        const unsigned index=p*16+k,code=(raw[16+(index/64)*32+index%32]>>((index/32%2)*4))&15;
        const unsigned element=k*16+row,operand=decoded[p].operand[element/2]>>((element%2)*16)&65535;
        assert(PioneerInt8Operand(int(code))==operand);qdot+=int(code)*input[index];sum+=input[index];
       }
       assert(IntegerAffine::PanelDomain(qdot,sum,factor,min));
       m.lr[0][lane]=Bits(float(qdot));m.lr[1][lane]=Bits(float(factor));m.lr[2][lane]=Bits(float(min));m.lr[3][lane]=Bits(float(sum));
       dot[parity][lane]+=int64_t(qdot)*factor;minimum[parity][lane]+=int64_t(sum)*min;
       volatile float c=Float(decoded[p].coefficient[row])*activation,mn=Float(decoded[p].minimum[row])*activation;
       volatile float product=c*float(qdot),correction=mn*float(sum),value=product+correction;
       legacy[parity][lane]+=value;
      }
      for(auto word:panels[parity].words)m.Op(word);assert(m.okay);
      for(unsigned lane=0;lane<32;lane++){
       assert(m.Read(4+parity*2,lane)==Bits(float(dot[parity][lane])));
       assert(m.Read(5+parity*2,lane)==Bits(float(minimum[parity][lane])));
      }
     }
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
     int panelSum=0;for(unsigned k=0;k<16;k++)panelSum+=input[p*16+k];
     stateMachine->Dot(p,input);assert(state.Panel(*stateMachine,p,panelSum));stateMachine->Closed();
#endif
    }
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
    assert(state.Finish(*stateMachine,Bits(activation)));stateMachine->Closed();
    assert(!state.failed && !state.active && state.next==16 && stateMachine->zeroUnpacks==1 && stateMachine->unpacks==1);
    for(unsigned r=109;r<112;r++)for(unsigned c=0;c<16;c++)assert(!(stateMachine->FloatDst(r,c)&0x7fffffff));
#endif
    for(unsigned parity=0;parity<2;parity++)for(unsigned lane=0;lane<32;lane++){
     second[parity][lane]=m.lr[4+parity*2][lane];savedMin[parity][lane]=m.lr[5+parity*2][lane];
    }
    for(unsigned parity=0;parity<2;parity++){
     uint32_t expected[32];
     for(unsigned lane=0;lane<32;lane++){
      const unsigned row=(lane%8)*2+parity;const auto* raw=rows[row];
      const uint32_t total=PioneerBlackholeFmaBits(Bits(float(saved[parity][lane])),0x3f800000,second[parity][lane]);
      assert(total==Bits(float(saved[parity][lane]+dot[parity][lane])));
      const float d=Half(raw[0]|unsigned(raw[1])<<8),dm=Half(raw[2]|unsigned(raw[3])<<8);
      m.lr[0][lane]=total;m.lr[1][lane]=Bits(d);m.lr[2][lane]=Bits(-dm);m.lr[3][lane]=Bits(activation);m.lr[4][lane]=0;
      assert(m.lr[5+parity*2][lane]==savedMin[parity][lane]);
      volatile float c=d*activation,mn=(-dm)*activation,product=c*Float(total),correction=mn*float(minimum[parity][lane]);
      volatile float value=product+correction;expected[lane]=Bits(0.f+value);
      changed+=expected[lane]!=Bits(legacy[parity][lane]);
     }
     for(auto word:finishes[parity].words)m.Op(word);assert(m.okay);
     for(unsigned lane=0;lane<32;lane++){
      assert(m.Read(0,lane)==expected[lane]);words++;
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
      if(lane<8)assert(stateMachine->FloatDst(108,lane*2+parity)==expected[lane]);
#endif
     }
    }
    jobs++;
   }
  }
 }
 assert(tensors==193 && jobs==3474 && words==222336 && changed);
 assert(model->Stable() && !fstat(fd,&after) && Same(before,after) && !fclose(file));
 printf("PASS ORIGINAL_BLOCK_INTEGER tensors%u sampledBlocks1737 changedInputJobs%u laneWords%u uniqueRowOutputs55584 changedFromOldK16%u;firstMiddleLastRowsAndKBlocks;fullSameOpenSHAStable;syntheticActivations duplicatedFourSubvectors;NEW_ROUNDING_NO_DST_NATIVE_INFERENCE_QUALITY_OR_SPEED\n",tensors,jobs,words,changed);
#ifdef PIONEER_Q38_BLOCK_INTEGER_STATE_ORIGINAL_TEST
 puts("PASS ORIGINAL_EMITTED_BLOCK_INTEGER_STATE tensors193_sampledBlocks1737_changedInputJobs3474_physicalDstWords222336_includingPadding;actualRawMetadataLoads_firstHalfStores_originalQdotReconstruction_55584uniqueRowOutputs;SYNTHETIC_COMPLETED_FPU_DOT_NOT_COMPILED_WORKER_NATIVE_QUALITY_OR_SPEED");
 (void)&Run;(void)&Check;(void)&Round;
#endif
}
