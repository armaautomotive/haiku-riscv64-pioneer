/* SPDX-License-Identifier: MIT */
// Distinct SHADER integration candidate,NOT a native/client/Work capability.
// Included after actual original NoC helpers;no replacement IO endpoints.
#if !defined(PIONEER_Q38_EXACT_Q4_WORK_IMAGE) || PIONEER_Q38_EXACT_Q4_WORK_IMAGE != 1 \
 || !defined(PIONEER_Q38_VECTOR_Q4_METADATA_IMAGE) || !defined(PIONEER_Q38_VECTOR_Q4_METADATA) \
 || !defined(PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH) || defined(PIONEER_Q38_MACRO_Q4_DECODE) \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error Separate original-source corrected-Q4 returning shader only
#endif
#ifdef PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE
#include "PioneerQ38PreparedQ4Work.inc"
#else
#include "PioneerQ38ExactQ4Original.h"
#include "PioneerQ38ExactQ4Affine.h"
// SAME already-saved6e000..b0000 private peer extent. Two complete raw
// batches end9ba80;new operand headers/data end9d140,below original input
// a0000. No cross-job cache or other service's live scratch is admitted.
static_assert(PioneerQ38TwinBatchPrefetchPlan::BufferLimit<0x9c030
 && 0x9c360<=0x9d030 && 0x9d140<0xa0000,"disjoint original NoC/native operands/input/canaries");
struct PioneerQ38ExactQ4WorkEmitter {
 unsigned metadataRow=128;
 __attribute__((always_inline)) inline void Op(unsigned word){
  if((word&~2U)==0x70130060)word=0x70130000|metadataRow|(word&2);
  else if((word&~2U)==0x70230064)word=0x70230000|(metadataRow+4)|(word&2);
  Emit(word);
 }
 __attribute__((always_inline)) inline void Config(unsigned index,unsigned value){
  // ONLY corrected primitive's fixed data descriptors are relocated.
  // Original metadata/NoC helpers use their ORIGINAL emitter,not this.
  if(index==76 && (value==0x8fff || value==0x900f || value==0x901f))value+=0xc04;
  if(index==124 && value==0x90ff)value=0x9d03;
  ::Config(index,value);
 }
 void Drain(){::Drain();}
 void SealData(){Fence();}
 unsigned OriginalWord(unsigned at){return *Words(at);}
 void Primary(unsigned i,unsigned value){Words(0x9c040)[i]=value;}
 void MaskWord(unsigned i,unsigned value){Words(0x9c340)[i]=value;}
 unsigned Mask(unsigned i){return Words(0x9c340)[i];}
 void Escape(unsigned i,unsigned value){Words(0x9c240)[i]=value;}
 void Seven(unsigned i,unsigned value){Words(0x9c140)[i]=value;}
};
__attribute__((noinline,noclone,section(".text.pioneer_q38_resident_service")))
static unsigned PioneerQ38ExactQ4Work(const volatile uint32_t* job){
 // Caller has already validated the immutable descriptor,ALL native
 // activation operands/scales and completed shared-input/source reads.
 const unsigned inputs=job[5],groups=job[7],blocks=inputs/256,first=job[6],source=job[8];
 if(job[1]!=12 || job[2]!=2304 || job[14]!=1)return 0;
 PioneerQ38ExactQ4WorkEmitter e;
 if(!PioneerQ38ExactQ4Corrected::Setup(e))block_fault_park();
 for(unsigned i=0;i<4;i++){
  Words(0x9c030)[i]=0;Words(0x9c130)[i]=0;Words(0x9c230)[i]=0;Words(0x9d030)[i]=0;
 }
 Fence();
 for(unsigned group=0;group<groups;group++){
  // Define COMPLETE metadata/accumulator physical halves/padding,not an
  // ALL-Dst clear followed by partial stores. The original forced-zero
  // source is the validated input region,as in the accepted worker.
  e.Drain();e.Config(50,0x100);VectorPanelEmitter init;
  PioneerQ38VectorDstInit(init,true,true,true,true);
  uint32_t expected=0;
  for(unsigned block=0;block<blocks;block++){
   const unsigned raw=TwinBatchTilePrefetchStage(12,inputs,groups,group,block,first,source,expected);
   const unsigned scale=Words(0x94000)[block];
   if(!PioneerQ38VectorQ4Metadata::FiniteBlock(Words(raw),2304))block_fault_park();
   // SAME original raw-prefix SIMD metadata products;no scalar HALF or
   // learned multiplication. Explicit no-flip imports leave banks free.
   e.Drain();e.Config(50,0x100);VectorPanelEmitter metadata;
   if(!PioneerQ38VectorQ4Metadata::Block(metadata,raw,scale))block_fault_park();
   // Metadata imports bytes into Dst;the matrix engine must be switched
   // back to explicit BF16 operands before the corrected native products.
   e.Config(1,0x60aa0000);
   for(unsigned p=0;p<16;p++){
    if(!PioneerQ38ExactQ4Original::Panel(e,raw,p))block_fault_park();
    uint32_t converted[8];int sum=0;
    for(unsigned i=0;i<8;i++){
     const unsigned word=Words(0xa0000)[block*128+p*8+i];unsigned lo=0,hi=0;
     if(!PioneerQ38ExactQ4Original::Activation(word&65535,lo) || !PioneerQ38ExactQ4Original::Activation(word>>16,hi))block_fault_park();
     converted[i]=lo|(hi<<16);
     if(!group)for(unsigned lane=0;lane<2;lane++){
      const unsigned v=(word>>(lane*16))&65535;sum+=int(v&1023)*(v&0x8000?-1:1);
     }
    }
    if(group)sum=int(Words(0x95000)[block*16+p]);
    else if(groups>1)Words(0x95000)[block*16+p]=unsigned(sum);
    if(sum<-2048 || sum>2048)block_fault_park();
    for(unsigned r=0;r<8;r++)for(unsigned i=0;i<8;i++)Words(0x9d040)[r*8+i]=converted[i];
    Fence();
    e.Config(65,0x00010001);e.Config(73,0x000f010f);e.Config(86,0x01000100);
    PioneerQ38ExactQ4Affine::PanelEmitter<PioneerQ38ExactQ4WorkEmitter> panel(e);
    if(!PioneerQ38ExactQ4Corrected::Run(panel))block_fault_park();
    e.metadataRow=128+(p/2)*8;
    PioneerQ38ExactQ4Affine::Affine(e,scale,sum); // FP32 dot,original perK16 updates
   }
  }
  e.Drain();e.Config(3,0);
  for(unsigned n=0;n<16;n++){
   const unsigned value=Words(0xffbd9b00)[n];
   if((value&0x7f800000)==0x7f800000)block_fault_park();
   Words(0x84000)[group*16+n]=value;
  }
  Fence();
 }
 return groups*blocks*16;
}
#endif
