#!/usr/bin/env python3
"""Neutral TEST declaration and separate oracle/entry role perimeter."""
import os
import pathlib
import subprocess
here=pathlib.Path(__file__).resolve().parent
api=here/'pioneer_q38_mixed_live_original_native.h'
count=0
def check(defines,okay,late=''):
 global count
 body=''.join('#define '+key+' '+value+'\n' for key,value in sorted(defines.items()))+'#include "'+str(api)+'"\n'+late
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-E','-P','-x','c++','-'],input=body,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=30)
 assert (result.returncode==0)==okay,(defines,late,result.stderr)
 count+=1
check({},True);check({'PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST':'1'},True)
for key in ('_KERNEL_MODE','PIONEER_ATTENTION_FREESTANDING','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_WORK_INFERENCE',
 'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE','PIONEER_Q38_MIXED_WORKER_IMAGE'):
 for value in ('0','1'):check({key:value},False)
 check({},False,'#define '+key+' 0\n#include "'+str(api)+'"\n')
for value in ('0','2'):check({'PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST':value},False)
check({},False,'#define PIONEER_Q38_MIXED_LIVE_ORIGINAL_NATIVE_TEST 0\n#include "'+str(api)+'"\n')
text=api.read_text()
for forbidden in ('MixedLiveOriginalSource','ReferenceProducts','CompareFinal','PioneerStoreContext','PioneerSessionIO'):
 assert forbidden not in text,forbidden
native=(here/'pioneer_q38_mixed_live_native_factory_test.cpp').read_text()
assert '#include "pioneer_q38_mixed_live_original_bytes.h"' not in native and 'CompareFinal(' not in native
print('PASS original-native neutral TEST header gates'+str(count)+'/defined0+late kernel,CPU-oracle,image,Work refused/only raw weight bytes+post-END actual words cross boundary;NO loadable/native-ready/model numeric grant')
