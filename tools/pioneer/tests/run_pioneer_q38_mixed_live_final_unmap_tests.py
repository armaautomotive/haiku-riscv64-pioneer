#!/usr/bin/env python3
"""Actual new VM-only adapter plus verbatim generic-method characterization."""
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
assert method.count('bool Unmap()')==1 and 'PioneerHostWriteIO::Unmap() && okay' in method
role=(kernel/'PioneerQ38MixedLiveNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==20
defines={key:'1' for key in parents}
defines.update({'PIONEER_DRIVER_BUILD_TAG':'0x51424c4e4f445256ULL','PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS':'1',
 'PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK':'1','PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE':'1'})
defines.update({key:'pioneer_mixed_final_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-final-unmap-',dir='/private/tmp') as temp:
 base=pathlib.Path(temp);(base/'session_unmap.inc').write_text(method+'\n')
 for name,flags in [('generic_unmap_characterization',['-DPIONEER_Q38_EMBEDDING_NATIVE=1']),('final_unmap',['-D'+k+'='+v for k,v in sorted(defines.items())]),('final_pci_state',['-D'+k+'='+v for k,v in sorted(defines.items())])]:
  binary=base/name
  subprocess.run([*common,*flags,'-I'+str(base),str(here/('pioneer_q38_mixed_live_'+name+'_test.cpp')),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
