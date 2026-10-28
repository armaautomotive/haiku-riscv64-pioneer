#!/usr/bin/env python3
"""Production QRPNODRV binding/retention with actual zero/QPE1; no QPX1 launch."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) in (3,4) and (len(sys.argv)==3 or sys.argv[3] in ('--initialize','--init-faults'))
initialization=len(sys.argv)==4
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
for path,name,digest in ((pathlib.Path(sys.argv[1]),'text.bin','39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'),
 (pathlib.Path(sys.argv[2]),'code.bin','c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1')):
 assert hashlib.sha256((path/name).read_bytes()).hexdigest()==digest
role=(kernel/'PioneerQ38RecurrentPrefixNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==21
defines={key:'1' for key in parents};defines['PIONEER_DRIVER_BUILD_TAG']='0x5152504e4f445256ULL'
defines.update({key:'pioneer_recurrent_binding_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
binding=(kernel/'PioneerQ38RecurrentPrefixNative.inc').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-recurrent-native-bind-',dir='/private/tmp') as temporary:
 base=pathlib.Path(temporary)
 # Adapt TEST-only endpoint names to the DISTINCT prefix type. Never
 # rewrite production scopes, factories, controllers, leases or producers.
 fixture=(here/'pioneer_q38_embedding_native_factory_test.cpp').read_text()
 fixture=fixture.replace('RecurrentBlock','RecurrentPrefix').replace('RECURRENT_BLOCK','RECURRENT_PREFIX')\
  .replace('recurrent_block','recurrent_prefix').replace('recurrentBlock','recurrentPrefix')
 def include_path(match):
  name=match[1];candidate=(here/name).resolve()
  return '#include "'+str(candidate)+'"' if candidate.exists() else match[0]
 fixture=re.sub(r'#include "([^"]+)"',include_path,fixture)
 (base/'prefix_embedding_factory_fixture.inc').write_text(fixture)
 for name,marker in (('scope','PIONEER_Q38_EMBEDDING_NATIVE_SCOPE'),('unsafe','PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE'),
  ('mapping','PIONEER_Q38_EMBEDDING_NATIVE_MAPPING'),('factory','PIONEER_Q38_EMBEDDING_NATIVE_FACTORY'),
  ('end','PIONEER_Q38_EMBEDDING_NATIVE_END'),('retain','PIONEER_Q38_RETAIN_MEMORY')):
  begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert source.count(begin)==source.count(end)==1
  (base/('embedding_factory_'+name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
 first='bool PioneerSessionIO::RecurrentPrefixMappingSourceScope()'
 start='[[maybe_unused]] static bool BindQ38RecurrentPrefixNative('
 end='#include "PioneerQ38RecurrentPrefixNativeInitialize.inc"'
 assert binding.count(first)==binding.count(start)==binding.count(end)==1
 (base/'recurrent_prefix_factory_bind.inc').write_text(binding.split(first,1)[0]+'\n#endif\n'+start+binding.split(start,1)[1].split(end,1)[0])
 start='#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT\n if(c.q38RecurrentPrefixNative || c.io.q38RecurrentPrefixMemoryMapAttempted)'
 assert source.count(start)==1
 (base/'recurrent_prefix_factory_end.inc').write_text(start+source.split(start,1)[1].split('\n#endif',1)[0]+'\n#endif\n')
 if initialization:
  # Verbatim complete production callbacks. The non-root MMIO view itself
  # remains a simulated endpoint,not a copied private ownership grant.
  def method(signature):
   assert source.count(signature)==1
   start=source.index(signature);at=source.index('{',start);depth=1;end=at+1
   while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
   return source[start:end]+'\n'
  (base/'recurrent_prefix_zero_callbacks.inc').write_text(''.join(method(s) for s in (
   'bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()',
   'bool PioneerSessionIO::NonRootHeldOwner()')))
 binary=base/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),'-I'+str(kernel),
  str(here/('pioneer_q38_recurrent_prefix_native_initialization_test.cpp' if initialization else 'pioneer_q38_recurrent_prefix_native_binding_test.cpp')),
  str(here/'pioneer_q38_embedding_execution_bridge.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
