/* SPDX-License-Identifier: MIT */
// Actual Channel receiver, in-memory socket only. No process or hardware.
#define PIONEER_FULL_OFFLOAD_CHANNEL_ONLY 1
#include "../pioneer_q38_full_offload_smoke.cpp"
#include "../PioneerQ38FullOffloadRateDeadline.h"
#include <cassert>
#include <cstring>

static void CheckPhaseDeadline(){
 using D=PioneerQ38FullOffloadRateDeadline;
 const char* prepared="Full-card Qwen27B upload 19522.4/19522.4 MiB. No CPU inference.";
 const char* original="Full-card Qwen27B upload 18500.4/18500.4 MiB. No CPU inference.";
 D d(100),invalid(-1);
 assert(!invalid.Valid() && !invalid.Observe(1000,prepared,0));
 assert(d.Valid() && d.Until()==7300 && !d.Started());
 assert(!d.Observe(UINT32_MAX,D::Running,101));
 assert(!d.Observe(1000,"Full-card Qwen27B upload 0.0/19522.4 MiB. No CPU inference.",102));
 assert(d.Observe(999,prepared,7100) && d.Until()==7300);
 assert(!d.Observe(1000,prepared,7301) && d.Until()==7300);
 assert(d.Observe(1000,prepared,7299) && d.Until()==8499);
 assert(!d.Observe(1000,prepared,8300) && d.Until()==8499);
 assert(!d.Observe(0,D::Running,8301) && d.Until()==8499);
 assert(!d.Observe(UINT32_MAX,D::Running,8500) && !d.Started());
 assert(d.Observe(UINT32_MAX,D::Running,8498) && d.Until()==10598 && d.Started());
 assert(!d.Observe(UINT32_MAX,D::Running,10001) && d.Until()==10598);
 assert(d.Observe(UINT32_MAX,"counter",10002) && d.Until()==10598);
 D other(0);assert(other.Observe(1000,original,1) && other.Until()==1201);
}

int main(){
 CheckPhaseDeadline();
 int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
 Channel c;c.fd=pair[0];c.id=7;A::Frame f;
 assert(!Progress(f));
 assert(!c.Next(f,Now()-1) && !strcmp(c.nextFailure,"deadline"));
 A::Frame wrong;wrong.id=8;wrong.kind=A::Progress;assert(A::Send(pair[1],wrong));
 assert(!c.Next(f,Now()+1) && !strcmp(c.nextFailure,"frame_id"));
 close(pair[1]);
 assert(!c.Next(f,Now()+1) && !strcmp(c.nextFailure,"connection_lost"));
 puts("PASS receiver deadline/frame-id/lost distinctions and bounded phase transitions;no hardware");
}
