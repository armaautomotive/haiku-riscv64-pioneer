/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentClock.h"
#include <cassert>
#include <cstdio>
struct Span {};
struct Context {Span* q38Span=nullptr;};
int main(){
 Span span,other;
 for(unsigned failure=0;failure<4;failure++){
  Context c;Span* fresh=&span;unsigned begins=0,checks=0;
  if(failure==1)fresh=nullptr;
  if(failure==2)c.q38Span=&other;
  auto begin=[&](Context& input){begins++;assert(!input.q38Span && fresh==&span);return failure!=3;};
  auto current=[&](){checks++;return true;};
  bool okay=PioneerQ38QgwdResidentClockBeforeSpan(c,fresh,begin,current);
  if(!failure){assert(okay && begins==1 && checks==1 && c.q38Span==&span && !fresh);}
  else {assert(!okay && !checks && begins==unsigned(failure==3));
   assert(c.q38Span==(failure==2?&other:nullptr));
   assert(fresh==(failure==1?nullptr:&span));}
 }
 Context c;Span* fresh=&span;unsigned began=0;
 assert(!PioneerQ38QgwdResidentClockBeforeSpan(c,fresh,[&](Context& x){began++;return !x.q38Span;},[]{return false;}));
 assert(began==1 && fresh==&span && !c.q38Span);
 puts("PASS deferred span attachment on success and all injected refusals; FAKE CLOCK ONLY");
}
