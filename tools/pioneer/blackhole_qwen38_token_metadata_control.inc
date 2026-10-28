/* SPDX-License-Identifier: MIT */
// Inside the actual experimental parent. Always start with immutable pos0
// templates, transform ON CARD, and stage before the real returning call.
static __attribute__((noinline,section(".conv.token"))) void TokenRecipes(unsigned from,unsigned to,unsigned first,unsigned count,bool gddr){
 const auto* p=Owner();
 for(unsigned slot=first;slot<first+count;slot++){
  Transfer(from+(slot-first)*5120,5120,false);
  if(!M::Recipe(reinterpret_cast<uint32_t*>(R::Scratch),p->state[Layer],slot,p->state[Position],p->state[Token]))scalar_fault_park();
  if(gddr)Transfer(to+(slot-first)*5120,5120,true);
  else for(unsigned i=0;i<1280;i++)Words(to+(slot-first)*5120)[i]=Words(R::Scratch)[i];
 }
}
static __attribute__((noinline,section(".conv.token"))) void Metadata(unsigned layer,unsigned package){
 const auto* p=Owner();const auto* o=p->outer;const unsigned position=p->state[Position];
 if(!L::Dense(layer)){
  TokenRecipes(package+L::P::RecipesOffset,o[57],0,8,true);
  unsigned worker=0,slot=0;
  for(unsigned at=0;at<L::P::ConvBytes;at+=16384){
   unsigned bytes=L::P::ConvBytes-at;if(bytes>16384)bytes=16384;
   Transfer(package+L::P::ConvOffset+at,bytes,false);
   for(unsigned i=0;i<bytes;i+=256){
    if(!M::Conv(reinterpret_cast<uint32_t*>(R::Scratch+i),layer,worker,slot,position))scalar_fault_park();
    if(++slot==6){slot=0;worker++;}
   }
   Transfer(o[58]+at,bytes,true);
  }
  if(worker!=120 || slot)scalar_fault_park();
  for(unsigned at=0,first=0;at<L::P::GdnBytes;at+=12288,first+=16){
   Transfer(package+L::P::GdnOffset+at,12288,false);
   for(unsigned i=0;i<16;i++)if(!M::Gdn(reinterpret_cast<uint32_t*>(R::Scratch+i*768),layer,first+i,position))scalar_fault_park();
   Transfer(o[59]+at,12288,true);
  }
 }else{
  TokenRecipes(package+L::DenseRecipes,D::Recipes,0,4,false);
  TokenRecipes(package+L::DenseFfn,o[68],4,3,true);
  for(unsigned at=0;at<96*256;at+=16384){
   unsigned bytes=96*256-at;if(bytes>16384)bytes=16384;
   Transfer(package+L::DenseFrames+at,bytes,false);
   for(unsigned i=0;i<bytes;i+=256)if(!M::Dense(reinterpret_cast<uint32_t*>(R::Scratch+i),layer,(at+i)>>8,position))scalar_fault_park();
   for(unsigned i=0;i<bytes/4;i++)Words(D::Frames+at)[i]=Words(R::Scratch)[i];
  }
 }
 Scope();
}
