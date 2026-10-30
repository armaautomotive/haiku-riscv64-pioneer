/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION)
#error State DATA recorder implementation is HOST TEST only,never native,CPU-oracle or Work
#endif
#define PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE 1
#include "pioneer_q38_native_state_test_bridge.h"
#include "pioneer_q38_state_snapshot_data.h"
struct Q38NativeStateTest::Impl {bool seen[2][64][4]={},failed=false,finished=false;unsigned records=0;std::string directory;};
Q38NativeStateTest::Q38NativeStateTest():impl(new Impl){}
Q38NativeStateTest::~Q38NativeStateTest(){delete impl;}
bool Q38NativeStateTest::Record(const char* directory,unsigned completed,unsigned layer,unsigned kind,const uint32_t* words,unsigned count){
 auto& s=*impl;
 if(s.failed || s.finished || !directory || !count || count!=Q38StateSnapshotData::Count(completed,layer,kind)
  || s.seen[completed-2][layer][kind] || (!s.directory.empty() && s.directory!=directory))return s.failed=true,false;
 if(!Q38StateSnapshotData::Save(directory,true,completed,layer,kind,words,count))return s.failed=true,false;
 s.directory=directory;s.seen[completed-2][layer][kind]=true;s.records++;return true;
}
bool Q38NativeStateTest::Finish(const char* directory){
 auto& s=*impl;
 if(s.failed || s.finished || !directory || s.directory!=directory || s.records!=176)return s.failed=true,false;
 for(unsigned completed=2;completed<=3;completed++)for(unsigned layer=0;layer<64;layer++)for(unsigned kind=0;kind<4;kind++)
  if(s.seen[completed-2][layer][kind]!=(Q38StateSnapshotData::Count(completed,layer,kind)!=0))return s.failed=true,false;
 if(!Q38StateSnapshotData::Finish(directory,true,s.records))return s.failed=true,false;s.finished=true;return true;
}
