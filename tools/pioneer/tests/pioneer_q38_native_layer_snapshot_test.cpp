/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#ifdef PIONEER_Q38_NATIVE_TRACE_FACADE_TEST
#define PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE 1
#include "pioneer_q38_native_trace_test_bridge.h"
using Trace=Q38NativeTraceTest;
#else
#include "PioneerQ38ObservedTrace.h"
using Trace=PioneerQ38ObservedTrace;
#endif
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
static void Start(Trace& trace,unsigned layers){
 std::array<uint32_t,5120> actual={};
 assert(trace.Embedding(0,248319,actual.data(),actual.size()));
 for(unsigned layer=0;layer<layers;layer++)assert(trace.Layer(0,layer,actual.data(),actual.size()));
}
int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned bad=0;bad<20;bad++){
  const std::string directory=std::string(argv[1])+"/case-"+std::to_string(bad);
  assert(!mkdir(directory.c_str(),0700));Trace trace;
  const std::string name=directory+"/INCOMPLETE-native-position-0-layers-1.bin";
  if(bad==0)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
  else if(bad==1){Start(trace,0);assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));}
  else{
   Start(trace,bad==6?2:1);
   if(bad==2)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,0));
   if(bad==3)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,65));
   if(bad==4)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),3,1));
   if(bad==5)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),1,1));
   if(bad==6)assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,2));
   if(bad==7){
    assert(trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
   }
   if(bad==8){
    assert(trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
    std::array<uint32_t,5120> actual={};assert(trace.Layer(0,1,actual.data(),actual.size()));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,3));
   }
   if(bad==9)assert(!trace.SnapshotNativeLayerDiagnostic(nullptr,0,1));
   if(bad==10)assert(!trace.SnapshotNativeLayerDiagnostic("RELATIVE-NOT-OPENED",0,1));
   if(bad==11)assert(!trace.SnapshotNativeLayerDiagnostic((directory+"/missing").c_str(),0,1));
   if(bad==12){
    const std::string link=directory+"/link";assert(!symlink(directory.c_str(),link.c_str()));
    assert(!trace.SnapshotNativeLayerDiagnostic(link.c_str(),0,1));
   }
   if(bad==13 || bad==14){
    const std::string sentinel=bad==13?name:directory+"/sentinel";
    FILE* file=fopen(sentinel.c_str(),"wbx");assert(file && fwrite("KEEP",1,4,file)==4 && !fclose(file));
    if(bad==14)assert(!symlink(sentinel.c_str(),name.c_str()));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
    file=fopen(sentinel.c_str(),"rb");char bytes[5]={};assert(file && fread(bytes,1,5,file)==4
     && !memcmp(bytes,"KEEP",4) && !fclose(file));
   }
   if(bad==15){
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,0));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
   }
   if(bad==16){
    assert(trace.SaveFirstLayerDiagnostic(directory.c_str()));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
   }
   if(bad==17){
    assert(trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
    assert(!trace.Complete() && !trace.SavedComplete() && !trace.Save(directory.c_str()));
   }
   if(bad==18){
    std::array<uint32_t,5120> nonfinite={};nonfinite[0]=0x7f800000;
    assert(!trace.Layer(0,1,nonfinite.data(),nonfinite.size()));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
   }
   if(bad==19){
    std::array<uint32_t,5120> next={};assert(trace.Embedding(1,17,next.data(),next.size()));
    assert(!trace.SnapshotNativeLayerDiagnostic(directory.c_str(),0,1));
   }
  }
  assert(!trace.Complete() && !trace.SavedComplete() && !trace.Save(directory.c_str()));
 }
 puts("PASS incomplete native-layer recorder/20 missing,wrong,skipped,duplicate,nonfinite,closed,premature,sticky,filesystem+symlink+no-overwrite refusals/no full-token promotion;SYNTHETIC DATA only");
}
