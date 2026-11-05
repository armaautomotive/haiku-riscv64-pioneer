/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_exact_q4_affine_machine.h"
int main(){
 using P=PioneerQ38ExactQ4Affine;unsigned jobs=0,dots=0,failures=0;
 for(unsigned width:{5120U,6144U,17408U})for(unsigned schedule=0;schedule<2;schedule++) {
  ExactAffineMachine m;m.delay[0]=schedule?1:100003;m.delay[1]=schedule?100003:1;
  for(unsigned pass=0;pass<2;pass++) {
   auto expected=ExactAffineFixture(m,width,37+schedule*3+pass*139);
   bool done=false;
   try{done=P::Run(m,width);}catch(uint32_t marker){fprintf(stderr,"AFFINE_UNKNOWN marker%08x p%u matrix%u meta%u stores%u cycle%u vectorOkay%u pending%u dirty%u math%u sfpu%u\n",marker,m.stageCalls,m.matrixOps,m.metadataLoads,m.affineStores,m.cycle,m.vector.okay,m.Pending(),m.dirty,m.mathPending,m.sfpuPending);throw;}
   assert(done);auto result=m.Results();
   if(result!=expected)for(unsigned n=0;n<16;n++)if(result[n]!=expected[n])fprintf(stderr,"AFFINE_MISMATCH k%u pass%u n%u got%08x expected%08x\n",width,pass,n,result[n],expected[n]);
   assert(result==expected && m.Clean());jobs++;dots+=width;
  }
  assert(m.stageCalls==2*width/16 && m.matrixOps==6*width/16 && m.metadataLoads==2*width/16 && m.zeroLoads==2
   && m.affineStores==4*width/16 && !m.visits[1] && !m.visits[2] && !m.visits[3]);
 }
 struct Inert {void Config(unsigned,unsigned){assert(false);}void Op(unsigned){assert(false);}void Drain(){assert(false);}
  void Seven(unsigned,unsigned){assert(false);}void Escape(unsigned,unsigned){assert(false);}unsigned Mask(unsigned){assert(false);return 0;}
  void SealData(){assert(false);}void ZeroData(){assert(false);}bool StagePanel(unsigned,unsigned&,int&){assert(false);return false;}};
 for(unsigned width:{0U,16U,256U,5119U,5121U,17409U}){Inert e;assert(!P::Run(e,width));failures++;}
 for(unsigned fault=0;fault<4;fault++) {
  ExactAffineMachine m;const auto expected=ExactAffineFixture(m,5120,37);bool refused=false;
  if(fault==0)m.skipZero=true;if(fault==1)m.broadClear=true;if(fault==2)m.wrongDotCast=true;if(fault==3)m.dropProducer=1;
  try{if(P::Run(m,5120)){assert(fault==2);assert(m.Results()!=expected);refused=true;}}catch(uint32_t marker){refused=marker==m.Unknown;}
  assert(refused);failures++;
 }
 printf("PASS_OFFBOARD_CORRECTED_Q4_FULLK_ORIGINAL_AFFINE jobs%u originalWeightProducts%u negativeControls%u;"
  "K5120_6144_17408 dualJobs_changedWeightsInputsMetadataScale;perK16_FP32_rounding_notK32;NO_HARDWARE_MODEL_TPS_CLAIM\n",jobs,dots*16,failures);
}
