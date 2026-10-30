/* SPDX-License-Identifier: MIT */
// Finite polling POLICY only; no elapsed-clock, model or hardware proof.
#include "../PioneerQ38PushedWorkerLayout.h"
#include <cassert>
#include <cstdio>
#include <climits>
#include <initializer_list>
namespace G=PioneerQ38PushedWorkerLayout;
int main(){
 assert(G::ReceiptPollBudget(1,5120)==65536 && G::ReceiptPollBudget(7,5120)==458752
  && G::ReceiptPollBudget(16,17408)==G::MaxReceiptPolls);
 unsigned cases=0,refusals=0;
 for(unsigned groups=1;groups<=16;groups++)for(unsigned inputs=256;inputs<=17408;inputs+=256){
  const unsigned expected=65536*groups*((inputs+5119)/5120),budget=G::ReceiptPollBudget(groups,inputs);
  assert(budget==expected && budget>=65536 && budget<=G::MaxReceiptPolls);
  if(groups>1)assert(budget>=G::ReceiptPollBudget(groups-1,inputs));
  if(inputs>256)assert(budget>=G::ReceiptPollBudget(groups,inputs-256));
  cases++;
 }
 for(unsigned groups:{0U,17U,65536U,UINT_MAX})for(unsigned inputs:{0U,256U,5120U,17408U,UINT_MAX}){
  assert(!G::ReceiptPollBudget(groups,inputs));refusals++;
 }
 for(unsigned groups=1;groups<=16;groups++)for(unsigned inputs:{0U,1U,255U,257U,17409U,17664U,UINT_MAX}){
  assert(!G::ReceiptPollBudget(groups,inputs));refusals++;
 }
 assert(cases==1088 && refusals==132);
 printf("PASS finite G3 receipt-poll policy/%u valid group+input combinations/%u zero+alignment+range+overflow refusals/max%u polls/no variable divide or host override;POLICY ONLY,not runtime/elapsed-time/native/Work/TPS qualification\n",cases,refusals,G::MaxReceiptPolls);
}
