#!/usr/bin/env python3
"""Separate recurrent final VM/PCI components;NO native or physical grant."""
import os
import pathlib
import re
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
body=source.split('struct PioneerSessionIO :',1)[1].split('struct PioneerResidentMathIO :',1)[0]
start=body.index('\tbool Unmap() {');end=body.index('\n\t}\n};',start)+len('\n\t}')
method=body[start:end]
assert method.count('bool Unmap()')==1 and 'RecurrentBlockFinalUnmap()' in method
role=(kernel/'PioneerQ38RecurrentBlockNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==21
defines={key:'1' for key in parents}
defines.update({'PIONEER_DRIVER_BUILD_TAG':'0x5152424e4f445256ULL','PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE':'1'})
defines.update({key:'pioneer_recurrent_final_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-final-unmap-',dir='/private/tmp') as temp:
 base=pathlib.Path(temp);(base/'session_unmap.inc').write_text(method+'\n')
 for name in ('final_unmap','final_pci_state'):
  binary=base/name
  subprocess.run([*common,*['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),
   str(here/('pioneer_q38_recurrent_block_'+name+'_test.cpp')),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 # Actual new role rejects missing/zero parents and old role aliases,
 # including defined0. These are preprocessor gates,not runtime proofs.
 include='#include "'+str(kernel/'PioneerQ38RecurrentBlockFinalReleaseRole.h')+'"\n'
 def gate(d,okay):
  r=subprocess.run([common[0],'-x','c++','-E','-','-o',os.devnull,*['-D'+k+'='+v for k,v in sorted(d.items())]],input=include,text=True,capture_output=True)
  assert (r.returncode==0)==okay,r.stderr
 gate(defines,True);refused=0
 for key in (*sorted(parents),'PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE'):
  for absent in (False,True):
   bad=dict(defines)
   if absent:del bad[key]
   else:bad[key]='0'
   gate(bad,False);refused+=1
 for key in ('PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT','PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE','PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE'):
  for value in ('0','1'):
   gate({**defines,key:value},False);refused+=1
 print('PASS DISTINCT QRBNODRV final-release role/gates refusals'+str(refused)+';NO old native/Work/image alias or conventional driver ABI')
