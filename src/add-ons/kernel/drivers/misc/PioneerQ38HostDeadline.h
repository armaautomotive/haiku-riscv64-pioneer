/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_HOST_DEADLINE_H
#define PIONEER_Q38_HOST_DEADLINE_H
#include <stdint.h>
// CPU clock DATA only,never a source/ready/owner/cleanup capability.
// One bounded interval,sticky uncertainty/expiry,NO reset or extension.
// The actual native owner must observe it BEFORE and AFTER bounded card
// operations,and stop ALL card IO as soon as an observation fails.
class PioneerQ38HostDeadline {
 uint64_t begin=0,previous=0;bool started=false,failed=false;
public:
 static constexpr uint64_t Microseconds=UINT64_C(30000000);
 PioneerQ38HostDeadline()=default;
 PioneerQ38HostDeadline(const PioneerQ38HostDeadline&)=delete;
 PioneerQ38HostDeadline& operator=(const PioneerQ38HostDeadline&)=delete;
 bool Begin(uint64_t now){
  if(started || failed || now>UINT64_MAX-Microseconds){failed=true;return false;}
  begin=previous=now;started=true;return true;
 }
 bool Observe(uint64_t now){
  if(!started || failed || now<previous || now-begin>=Microseconds){failed=true;return false;}
  previous=now;return true;
 }
};
#endif
