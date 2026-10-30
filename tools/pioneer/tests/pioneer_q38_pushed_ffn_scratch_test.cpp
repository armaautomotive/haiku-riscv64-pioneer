/* SPDX-License-Identifier: MIT */
// DATA lifetime/alias regression only. No simulated processor state or native IO.
#include "../PioneerQ38GlobalChainLayout.h"
#include "../PioneerQ38TokenPushedFootprint.h"
#include <cassert>
#include <cstdio>
#include <vector>
namespace D=PioneerQ38MixedLiveBlockRootLayout;
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
namespace G=PioneerQ38PushedWorkerLayout;
namespace S=PioneerQ38TokenPushedL1;
static unsigned Pattern(unsigned address){return 0x9e3779b9U*address+0xa5a5a5a5U;}
#endif
int main(){
 constexpr unsigned words=3*D::RecipeStride/4;
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS)
 static_assert(PioneerQ38SelectedWorkerPushed==1 && D::FfnCopies==0x108000,"exact G3 scratch selection");
 static_assert(unsigned(D::FfnCopies)!=unsigned(G::Mailbox),"retained ordinary-matrix receipts remain live");
 std::vector<unsigned> memory(0x180000/4);
 for(unsigned at=0;at<0x180000;at+=4)memory[at/4]=Pattern(at);
 // Model the actual CaptureFfn copy's FULL three strides,including padding.
 // Keep all119 receipts arbitrary;there is no fabricated ready/completion.
 for(unsigned i=0;i<words;i++)memory[D::FfnCopies/4+i]=memory[D::FfnRecipes/4+i];
 for(unsigned at=0;at<0x180000;at+=4){
  assert(S::SnapshotContains(0,at)==(at==0x6000 || (at>=0x40000 && at<0x60008)
   || (at>=0x64000 && at<0x6c000) || (at>=0x6e000 && at<0x70000)
   || (at>=0x74000 && at<0x17f800) || (at>=0x17ffc0 && at<0x17ffe0)));
  const bool copy=at>=D::FfnCopies && at<D::FfnCopies+words*4;
  assert(memory[at/4]==Pattern(copy?D::FfnRecipes+at-D::FfnCopies:at));
  if(copy)assert(S::SnapshotContains(0,at));
 }
 for(unsigned w=1;w<=119;w++)for(unsigned i=0;i<64;i++){
  const unsigned at=G::Slot(w)+i*4;assert(memory[at/4]==Pattern(at) && S::SnapshotContains(0,at));
 }
 // The previous allocation would overwrite60 complete retained receipts.
 unsigned oldClobbered=0;
 for(unsigned at=G::Mailbox;at<G::Mailbox+words*4;at+=4){
  assert(Pattern(at)!=Pattern(D::FfnRecipes+at-G::Mailbox));oldClobbered++;
 }
 assert(oldClobbered==3840 && words==3840);
 printf("PASS G3 FFN scratch DATA/three full strides%u words/all119 retained receipt slots and live attention frames unchanged/old allocation would clobber%u words/current ROOT journal covers all writes;NOT actual instructions/native/model/TPS proof\n",words,oldClobbered);
#else
 static_assert(PioneerQ38SelectedWorkerPushed==0 && D::FfnCopies==0x118000,"old profiles retain their exact geometry");
 assert(words==3840);puts("PASS non-G3 FFN scratch DATA/legacy118000 unchanged;NOT execution proof");
#endif
}
