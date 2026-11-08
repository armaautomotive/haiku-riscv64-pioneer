/* SPDX-License-Identifier: MIT */
// Instantiate the real prepared QGW8 pages through the shared full lifecycle.
// The transport always refuses open: compile-only, no model or device access.
#define PIONEER_Q38_QGW8_FULL_SESSION_INSPECTION 1
#include "pioneer_q38_qgw8_upload_session_compile.cpp"
#include "../PioneerQ38HybridFullSession.h"
bool CompileQgw8FullSession(Prepared& pages,const PioneerQ38CanonicalModelPlan& canonical,
 NoDeviceTransport& io,const uint32_t* input,uint32_t* output){
 PioneerQ38HybridFullSession<Prepared,NoDeviceTransport> session(pages,io);unsigned count=0;
 return session.Begin(canonical,input,1,1) && session.Poll() && session.Finish()
  && session.CopyGeneratedTokens(output,32,count);
}
