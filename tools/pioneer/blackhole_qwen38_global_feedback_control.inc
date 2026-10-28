/* SPDX-License-Identifier: MIT */
// Inside the genuine global parent, after ALL973 output jobs close. This
// prepares a next input ON CARD but deliberately does not claim token commit.
namespace F=PioneerQ38GlobalFeedbackLayout;
static void FeedbackSeal(const uint32_t* h){
 InputSeal();HeadSeal(h);
 for(unsigned i=0;i<5120;i++)if(Words(F::Local)[i]!=Words(F::Shadow)[i]
  || (Words(F::Local)[i]&0x7f800000)==0x7f800000)scalar_fault_park();
 HeadSeal(h);
}
static __attribute__((noinline)) void Feedback(const uint32_t* h,unsigned token,uint32_t* result){
 auto* owner=Owner();uint32_t plan[8];
 if(!F::Describe(token,owner->outer[4]+1,owner->state[Bank],owner->state[Workspace],plan))scalar_fault_park();
 InputSeal();HeadSeal(h);Words(R::Status)[5]=0x300;
 // ALL20 original tiles are decoded and checked BEFORE either output write.
 for(unsigned block=0;block<20;block++){
  Packet(h,PioneerQ38ProjectionRoute(plan[4]),plan[5]+block*2304,2304,false);
  for(unsigned i=0;i<576;i++)Words(F::Raw)[i]=Words(R::Scratch)[i];
  HeadSeal(h);if(!q38_global_chain_feedback_decode(block,plan[6]))scalar_fault_park();HeadSeal(h);
 }
 for(unsigned i=0;i<5120;i++)Words(F::Shadow)[i]=Words(F::Local)[i];
 FeedbackSeal(h);
 for(unsigned at=0;at<20480;at+=16384){
  unsigned bytes=20480-at;if(bytes>16384)bytes=16384;
  for(unsigned i=0;i<bytes/4;i++)Words(R::Scratch)[i]=Words(F::Local+at)[i];
  Packet(h,PioneerQ38ProjectionRoute(plan[2]),plan[3]+at,bytes,true);
 }
 // Close both writes by reading the actual GDDR result, never by a host hint.
 for(unsigned at=0;at<20480;at+=16384){
  unsigned bytes=20480-at;if(bytes>16384)bytes=16384;
  Packet(h,PioneerQ38ProjectionRoute(plan[2]),plan[3]+at,bytes,false);
  for(unsigned i=0;i<bytes/4;i++)if(Words(R::Scratch)[i]!=Words(F::Local+at)[i])scalar_fault_park();
 }
 FeedbackSeal(h);
 const unsigned completed[]={F::Magic,1,token,plan[1],plan[2],plan[3],plan[4],plan[5],plan[6],5120,22,2,1040,320,0,1};
 for(unsigned i=0;i<16;i++)result[i]=completed[i];
}
static void PublishFeedback(const uint32_t* h,const uint32_t* result){
 // Reached only AFTER actual global STOP of all119 workers. The record is
 // explicitly DIAGNOSTIC/uncommitted; no next-state or inference grant.
 FeedbackSeal(h);for(unsigned i=0;i<64;i++)Words(F::Receipt)[i]=0;
 for(unsigned i=1;i<16;i++)Words(F::Receipt)[i]=result[i];
 for(unsigned i=0;i<16;i++)Words(F::Receipt)[16+i]=Owner()->boot[16+i];
 for(unsigned i=0;i<8;i++){
  Words(F::Receipt)[32+i]=Owner()->boot[8+i];Words(F::Receipt)[40+i]=Owner()->state[Counts+i];
 }
 Fence();Words(F::Receipt)[0]=F::Magic;Fence();
}
