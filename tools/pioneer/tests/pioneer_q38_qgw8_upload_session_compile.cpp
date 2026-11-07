/* SPDX-License-Identifier: MIT */
// Instantiate actual QGW8 source/session types; no execution or real IO.
#include "pioneer_q38_hybrid_head_prepared_actual_compile.cpp"
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_QGW8_USER_SESSION_INSPECTION 1
#include "../PioneerQ38Qgw8UploadSessionInspection.h"
struct NoDeviceTransport {
 int Open(){return -1;}int Close(int){return 0;}
 bool Call(int,unsigned,void*,size_t){return false;}
 bool Control(int,PioneerQ38Qgw8Upload::Packet&){return false;}
 bool OutputSafe(const void*,size_t)const{return true;}
};
bool CompileQgw8Session(Prepared& pages,const PioneerQ38CanonicalModelPlan& canonical,NoDeviceTransport& io,std::string& error){
 PioneerQ38Qgw8UploadSessionInspection<Prepared,NoDeviceTransport> session(pages,io);
 uint32_t manifest[session.RootManifestWords],boot[64]={},head[128];
 return session.Begin(canonical,error) && session.RootManifest(manifest,session.RootManifestWords)
  && session.HeadMetadata(boot,64,0,head,128) && session.Finish(error);
}
