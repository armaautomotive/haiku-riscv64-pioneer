#!/usr/bin/env python3
"""Returning IMAGE and standalone-layout isolation; no compiler/card launches."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv)==1 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;role=here.parent/'PioneerQ38WideGdnReturningImageRole.h'
parents=('PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE','PIONEER_Q38_RGDN_COMBINED_STAGE',
         'PIONEER_Q38_MIXED_CAPSULE_IMAGE','PIONEER_Q38_RESIDENT_GDN_OPERATOR_IMAGE',
         'PIONEER_Q38_RESIDENT_OPERATOR_IMAGE','PIONEER_Q38_FFN_PERSISTENT','PIONEER_Q38_PROJECTION_PERSISTENT')
base=dict.fromkeys(parents,'1');positive=negative=cli=0
include=lambda p:'#include "'+str(p)+'"\n'
def syntax(values,okay,body=None):
    global positive,negative
    source=''.join('#define '+key+' '+value+'\n' for key,value in sorted(values.items()))
    source+=body if body is not None else include(role)+include(role)
    result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Werror','-fsyntax-only','-x','c++','-'],input=source,text=True,capture_output=True)
    assert (result.returncode==0)==okay,(values,body,result.stderr)
    if okay:positive+=1
    else:
        assert 'error:' in result.stderr;negative+=1
syntax(base,True)
for key in parents:
    for value in (None,'0','2'):
        bad=base.copy()
        if value is None:del bad[key]
        else:bad[key]=value
        syntax(bad,False)
    syntax(base,False,include(role)+'#undef '+key+'\n'+include(role))
foreign=set()
for path in (role,here.parent/'PioneerQ38ResidentGdnOperatorImageRole.h',here.parent/'PioneerQ38ResidentOperatorImageRole.h'):
    foreign|=set(re.findall(r'defined\(([A-Z_][A-Z0-9_]*)\)',path.read_text()))
for key in sorted(foreign-set(parents)):
    for value in ('0','1'):
        syntax({**base,key:value},False)
        syntax(base,False,include(role)+'#define '+key+' '+value+'\n'+include(role))
layout=here.parent/'PioneerQ38WideDeltaLocalLayout.h'
standalone={'PIONEER_Q38_WIDE_DELTA_LOCAL_IMAGE':'1'}
syntax(standalone,True,include(layout)+'static_assert(PioneerQ38WideDeltaLocalLayout::Descriptor==0x74000);\n')
syntax({**base,**standalone},True,include(layout)+'static_assert(PioneerQ38WideDeltaLocalLayout::Descriptor==0x74700);\n')
syntax({**base,**standalone},False,include(layout)+'#undef PIONEER_Q38_WIDE_GDN_RETURNING_IMAGE\n'+include(layout))
syntax(standalone,False,include(layout)+''.join('#define '+key+' 1\n' for key in parents)+include(layout))
with tempfile.TemporaryDirectory(prefix='pioneer-q38-wide-gdn-build-gates-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp);builder=here.parent/'build_qwen38_wide_gdn_worker_image.py'
    for args in ([],['/unopened/sdk'],['relative-sdk',str(root/'bad')],['/unopened/sdk','relative-out'],
                 ['/unopened/sdk',str(root/'bad'),'extra'],['--inspect-candidate','--inspect-candidate','/unopened/sdk',str(root/'bad')]):
        result=subprocess.run([sys.executable,str(builder),*args],capture_output=True)
        assert result.returncode and not list(root.iterdir());cli+=1
    for key in (*parents,'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0','1'):
            output=root/'MUST_NOT_EXIST'
            result=subprocess.run([sys.executable,str(builder),'/unopened/sdk',str(output)],capture_output=True,env={**os.environ,key:value})
            assert result.returncode and not output.exists() and not list(root.iterdir());cli+=1
print(f'PASS returning-wide IMAGE/{positive} positives/{negative} missing,zero,two,late,HOST,DATA,native,CPU+Work refusals/{cli} BUILD CLI refusals before output/standalone-returning mailbox bind cannot change;NOT native lifecycle,model,hardware or speed grant')
