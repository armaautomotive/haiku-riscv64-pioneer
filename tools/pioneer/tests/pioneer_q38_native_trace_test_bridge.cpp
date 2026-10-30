/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Observed trace implementation is HOST TEST only,never kernel/native/Work
#endif
#define PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE 1
#include "pioneer_q38_native_trace_test_bridge.h"
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#include "PioneerQ38ObservedTrace.h"
struct Q38NativeTraceTest::Impl {PioneerQ38ObservedTrace data;};
Q38NativeTraceTest::Q38NativeTraceTest():impl(new Impl){}
Q38NativeTraceTest::~Q38NativeTraceTest(){delete impl;}
bool Q38NativeTraceTest::Embedding(unsigned p,unsigned t,const uint32_t* v,unsigned n){return impl->data.Embedding(p,t,v,n);}
bool Q38NativeTraceTest::Layer(unsigned p,unsigned l,const uint32_t* v,unsigned n){return impl->data.Layer(p,l,v,n);}
bool Q38NativeTraceTest::Vocabulary(unsigned p,unsigned first,const uint32_t* v,unsigned n){return impl->data.Vocabulary(p,first,v,n);}
bool Q38NativeTraceTest::Commit(unsigned p,const uint32_t* v,unsigned n){return impl->data.Commit(p,v,n);}
bool Q38NativeTraceTest::Complete()const{return impl->data.Complete();}
bool Q38NativeTraceTest::SavedComplete()const{return impl->data.SavedComplete();}
unsigned Q38NativeTraceTest::Embeddings()const{return impl->data.Embeddings();}
unsigned Q38NativeTraceTest::Layers(unsigned p)const{return impl->data.Layers(p);}
bool Q38NativeTraceTest::Save(const char* path){return impl->data.Save(path);}
bool Q38NativeTraceTest::SaveFirstLayerDiagnostic(const char* path){return impl->data.SaveFirstLayerDiagnostic(path);}
bool Q38NativeTraceTest::SnapshotFirstLayerDiagnostic(const char* path){return impl->data.SnapshotFirstLayerDiagnostic(path);}
