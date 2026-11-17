/* SPDX-License-Identifier: MIT */
// Actual Channel receiver, in-memory socket only. No process or hardware.
#define PIONEER_FULL_OFFLOAD_CHANNEL_ONLY 1
#include "../pioneer_q38_full_offload_smoke.cpp"
#include <cassert>
#include <cstring>

int main(){
 int pair[2];assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
 Channel c;c.fd=pair[0];c.id=7;A::Frame f;
 assert(!Progress(f));
 assert(!c.Next(f,Now()-1) && !strcmp(c.nextFailure,"deadline"));
 A::Frame wrong;wrong.id=8;wrong.kind=A::Progress;assert(A::Send(pair[1],wrong));
 assert(!c.Next(f,Now()+1) && !strcmp(c.nextFailure,"frame_id"));
 close(pair[1]);
 assert(!c.Next(f,Now()+1) && !strcmp(c.nextFailure,"connection_lost"));
 puts("PASS receiver deadline/frame-id/lost distinctions;local socket only");
}
