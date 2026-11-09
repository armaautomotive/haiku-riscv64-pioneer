/* SPDX-License-Identifier: MIT */
// Separate pre-device diagnostic source; never Work inference or kernel.
#ifndef PIONEER_QGW8_HEAD_BAND_SOURCE_H
#define PIONEER_QGW8_HEAD_BAND_SOURCE_H
#if !defined(PIONEER_QGW8_HEAD_SOURCE_DIAGNOSTIC) || PIONEER_QGW8_HEAD_SOURCE_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error Head-band source is isolated diagnostic only
#endif
#define PIONEER_QGW8_HEAD_EXPORT_VERIFY 1
#define main Qgw8HeadExportVerifyMain
#include "tests/pioneer_q38_qgw8_head_export_verify.cpp"
#undef main
#undef PIONEER_QGW8_HEAD_EXPORT_VERIFY

class PioneerQgw8HeadBandSource {
 static constexpr const char* Snapshot="/boot/home/pioneer-qgw8-head130-20261108-a/cpu-hidden.bin";
 const char* model=nullptr;struct stat modelIdentity={},snapshotIdentity={};bool attempted=false,prepared=false;
 static bool Identity(const char* path,struct stat& s,uint64_t size){
  return path && path[0]=='/' && !lstat(path,&s) && S_ISREG(s.st_mode) && s.st_nlink==1
   && s.st_uid==geteuid() && !(s.st_mode&0022) && uint64_t(s.st_size)==size;
 }
 static bool Same(const struct stat& a,const struct stat& b){
  return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size && a.st_mtime==b.st_mtime && a.st_ctime==b.st_ctime;
 }
 bool Stable()const{
  struct stat m={},s={};return Identity(model,m,PioneerQwen38Gguf::FullBytes)
   && Identity(Snapshot,s,(16+2*5120+1)*4) && Same(m,modelIdentity) && Same(s,snapshotIdentity);
 }
public:
 bool Ready()const{return prepared && Stable();}
 bool Prepare(const char* path,std::string& error){
  if(attempted){error="Head source is one-shot.";return false;}attempted=true;model=path;
  if(!Identity(model,modelIdentity,PioneerQwen38Gguf::FullBytes) || !Identity(Snapshot,snapshotIdentity,(16+2*5120+1)*4)){
   error="Head original or reference snapshot identity refused.";return false;
  }
  char name[]="head-source",*argv[]={name,const_cast<char*>(model),const_cast<char*>(Snapshot),nullptr};
  if(Qgw8HeadExportVerifyMain(3,argv) || !Stable()){
   error="Head original, reference, payload or stability refused.";return false;
  }
  prepared=true;error.clear();return true;
 }
};
#endif
