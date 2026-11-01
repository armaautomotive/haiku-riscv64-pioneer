#!/usr/bin/env python3
"""Failed-reply USER roles only,not a native/device/Work grant."""
import os
import pathlib
import re
import subprocess
import sys
assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
role=here.parent/'PioneerQ38FrontierFailureReportRole.h'
primary='PIONEER_Q38_TOKEN_FRONTIER_FAILURE_REPORT_CLIENT'
base={primary:'1','PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING':'1',
      'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT':'1','PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT':'1',
      'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT':'1'}
foreign=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',role.read_text()))-set(base)
positive=negative=0
include='#include "'+str(role)+'"\n'
def syntax(values,okay,body=None,selection=1):
    global positive,negative
    text=''.join('#define '+k+' '+v+'\n' for k,v in sorted(values.items()))
    text+=body if body is not None else include+include+f'static_assert(PioneerQ38FrontierFailureReportSelection=={selection});\n'
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-fsyntax-only','-x','c++','-'],
                          input=text,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:assert 'error:' in result.stderr;negative+=1
syntax({},True,selection=0);syntax(base,True)
for key in sorted(base):
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        syntax(bad,False)
    syntax(base,False,include+'#undef '+key+'\n#define '+key+' 0\n'+include)
for key in sorted(foreign):
    for value in ('0','1'):syntax({**base,key:value},False)
    syntax(base,False,include+'#define '+key+' 0\n'+include)
syntax({},False,include+''.join('#define '+k+' 1\n' for k in sorted(base))+include)
syntax(base,False,include+'#undef '+primary+'\n'+include)
for value in ('0','1'):syntax({'PIONEER_Q38_FRONTIER_FAILURE_REPORT_PARENT_BINDING':value},False)
syntax(base,True,'#include "'+str(here.parent/'PioneerQ38FrontierFailureReportCli.h')+'"\n')
syntax({},False,'#include "'+str(here.parent/'PioneerQ38FrontierFailureReportCli.h')+'"\n')
print(f'PASS failed-reply distinct USER/{positive} positives/{negative} missing,zero,two,late,native,shader,profile,CPU+Work refusals;NO packet provenance,driver,lifecycle,hardware or TPS grant')
