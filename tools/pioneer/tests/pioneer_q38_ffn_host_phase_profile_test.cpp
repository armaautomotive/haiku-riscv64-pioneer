/* SPDX-License-Identifier: MIT */
#include <cassert>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnHostPhaseProfile.h"
int main(){
 PioneerQ38FfnHostPhaseProfile p;assert(p.Pristine() && !p.Complete() && !p.Total());
 for(unsigned i=0;i<Q38_FFN_HOST_PHASES;i++)p.Add(i,100,100+i);
 p.polls=1;assert(p.Complete() && p.Total()==28 && !p.Pristine());
 p.Add(Q38_FFN_OUTPUT,200,202);assert(!p.Complete() && p.Total()==30);
 p.Clear();assert(p.Pristine());p.Add(8,0,0);assert(p.invalid && !p.Complete() && !p.Total());
 p.Clear();p.Add(0,2,1);assert(p.invalid && !p.Complete());
 p.Clear();p.us[0]=UINT64_MAX;p.Add(0,0,1);assert(p.invalid && !p.Total());
 p.Clear();p.calls[0]=UINT32_MAX;p.Add(0,0,0);assert(p.invalid && !p.Complete());
 p.Clear();p.polls=1;for(unsigned i=0;i<Q38_FFN_HOST_PHASES;i++)p.calls[i]=1;
 p.us[0]=UINT64_MAX;p.us[1]=1;assert(!p.Complete() && !p.Total());
 p.Clear();p.polls=1;for(unsigned i=0;i<Q38_FFN_HOST_PHASES;i++)p.Add(i,0,0);
 assert(p.Complete() && !p.Total());// zero fixture intervals are valid DATA,not measured hardware performance
}
