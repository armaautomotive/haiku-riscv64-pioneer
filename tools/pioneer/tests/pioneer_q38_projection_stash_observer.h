/* SPDX-License-Identifier: MIT */
// TEST-ONLY observation of the reserved future consumer's private proof.
// No setters/claim shortcuts,device endpoints or public production API.
#ifndef PIONEER_Q38_TEST_PROJECTION_STASH_OBSERVER_H
#define PIONEER_Q38_TEST_PROJECTION_STASH_OBSERVER_H
#if defined(PIONEER_Q38_PROJECTION_RESULT_STASH) && !defined(PIONEER_Q38_RESIDENT_DECODER_OWNER)
class PioneerQ38ResidentDecoderOwner {
public:
 static bool Completed(const PioneerQ38ProjectionGraphOwner& owner,const PioneerQ38ProjectionMutableBanks& graph,
  const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionIdentity& id,unsigned token,unsigned position,unsigned bank,unsigned role){
  return owner.MatchesCompleted(graph,plan,id,token,position,bank,role);
 }
};
#endif
#endif
