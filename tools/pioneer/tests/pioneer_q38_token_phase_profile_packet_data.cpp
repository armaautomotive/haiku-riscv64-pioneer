/* SPDX-License-Identifier: MIT */
// HOST DATA cross-check only: no FD, native owner, clock or accelerator IO.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <inttypes.h>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenPhaseProfilePacket.h"
int main(){
 for(;;){
  uint32_t words[96]={};
  const int first=scanf("%" SCNu32,&words[0]);
  if(first==EOF)return 0;
  if(first!=1)return 2;
  for(unsigned i=1;i<96;i++)if(scanf("%" SCNu32,&words[i])!=1)return 2;
  PioneerQ38TokenPhaseProfileControl::Packet p;
  static_assert(sizeof(p)==sizeof(words),"whole immutable DATA packet");
  memcpy(&p,words,sizeof(p));
  printf("%u\n",unsigned(PioneerQ38TokenPhaseProfileControl::ReplyShape(p)));
 }
}
