#!/usr/bin/env python3
"""Immutable DATA mode/count coherence; no source/native/launch authority."""
import os
import pathlib
import subprocess
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
include='#include "'+str(kernel/'PioneerQ38MixedLivePrograms.h')+'"\n'
foot='#include "'+str(kernel/'PioneerQ38MixedLiveFootprint.h')+'"\n'
sig='#include "'+str(kernel/'PioneerQ38MixedLiveSigmoidPrograms.h')+'"\n'
flag='PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS'
count=0
def check(text,expected):
 global count
 result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsyntax-only','-x','c++','-'],
  input=text,text=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,timeout=30)
 assert (result.returncode==0)==expected,(text,result.stderr)
 count+=1
check(include+'static_assert(TT_Q38_MIXED_LIVE_ROOT_CODE_WORDS==12581 && TT_Q38_MIXED_LIVE_CODE_WORDS==2630224,"default");',True)
check('#define '+flag+' 1\n'+include+'static_assert(TT_Q38_MIXED_LIVE_ROOT_CODE_WORDS==12752 && TT_Q38_MIXED_LIVE_CODE_WORDS==2630395,"sigmoid");',True)
for value in ('0','2','-1'):check('#define '+flag+' '+value+'\n'+include,False)
check(sig,False)
check('#define '+flag+' 1\n'+sig,True)
for first,second in ((include,include),(include,foot),(foot,include),(foot,foot)):
 check(first+'#define '+flag+' 1\n'+second,False)
 check('#define '+flag+' 1\n'+first+'#undef '+flag+'\n'+second,False)
check(include+'#define '+flag+' 0\n'+include,False)
print('PASS mixed program DATA selection gates'+str(count)+'/exact distinct counts+arrays/missing defined0 unknown late selection+removal rejected;NO native/Work/launch permission')
