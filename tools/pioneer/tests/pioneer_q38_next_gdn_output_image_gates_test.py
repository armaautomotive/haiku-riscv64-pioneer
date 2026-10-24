#!/usr/bin/env python3
"""Offline image wrapper roles/defined-zero isolation; NOT native gates."""
import os
import pathlib
import re
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent.parent
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cxx=os.environ.get('CXX','c++')
common=(here/'PioneerQ38NextGdnOutputImageOnly.h').read_text()
forbidden=set(re.findall(r'defined\(([^)]+)\)',common))
assert len(forbidden)>=30
roles={
 'input':({'PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE':'1','PIONEER_Q38_PROJECTION_PERSISTENT':'1'},'#define PIONEER_ATTENTION_FREESTANDING'),
 'gather':({'PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE':'1'},'#define PIONEER_ATTENTION_FREESTANDING'),
 'graph':({'PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT':'1'},'#define PIONEER_ATTENTION_FREESTANDING'),
 'peer':({'PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE':'1'},'#define PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE 1'),
}
cases=0
def check(text,flags,okay):
 global cases
 result=subprocess.run([cxx,'-E','-x','c++','-I',str(here),*['-D'+k+'='+v for k,v in flags.items()],'-'],
  input=text,capture_output=True,text=True,env=clean,timeout=5)
 assert (result.returncode==0)==okay,(flags,result.stderr)
 cases+=1
for role,(valid,delimiter) in roles.items():
 source=(here/('blackhole_qwen38_next_gdn_output_'+role+'.cpp')).read_text()
 assert delimiter in source
 prefix=source.split(delimiter,1)[0]
 check(prefix,valid,True)
 for key in valid:
  for value in (None,'0','2','-1'):
   flags=dict(valid)
   if value is None:del flags[key]
   else:flags[key]=value
   check(prefix,flags,False)
 for key in forbidden | {'PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT','PIONEER_Q38_GDN_OUTPUT_INPUT_IMAGE',
  'PIONEER_Q38_NEXT_GDN_OUTPUT_INPUT_IMAGE','PIONEER_Q38_NEXT_GDN_OUTPUT_GATHER_IMAGE','PIONEER_Q38_NEXT_GDN_OUTPUT_GRAPH_ROOT'}:
  if key in valid:continue
  for value in ('0','1'):check(prefix,{**valid,key:value},False)
for key in forbidden:
 for value in ('0','1'):
  check('#include "PioneerQ38NextGdnOutputImageOnly.h"\n#define '+key+' '+value+'\n#include "PioneerQ38NextGdnOutputImageOnly.h"\n',{},False)
with tempfile.TemporaryDirectory(prefix='pioneer-qop2-image-gates-') as temporary:
 for role in ('input','graph'):
  builder=here/('build_qwen38_next_gdn_output_'+role+'_image.sh')
  # Only PIONEER_* environment values are builder selectors. _KERNEL_MODE
  # is tested above as a compiler definition, not a shell environment flag.
  for number,key in enumerate(sorted(k for k in forbidden | {'PIONEER_UNKNOWN_SELECTOR'} if k.startswith('PIONEER_'))):
   output=pathlib.Path(temporary)/(role+str(number))
   result=subprocess.run(['sh',str(builder),'/SDK-MUST-NOT-BE-TOUCHED',str(output)],
    capture_output=True,text=True,env={**clean,key:'1'},timeout=5)
   assert result.returncode==2 and not result.stdout and not output.exists(),(key,result.stderr)
   cases+=1
print(f'PASS QOP2/QOI2/QOG2 offline wrapper roles/defined-zero/native+CPU-reference+Work isolation/early ambient-builder refusals checks{cases};NO_NATIVE_ADMISSION_OR_SPEED_PROOF')
