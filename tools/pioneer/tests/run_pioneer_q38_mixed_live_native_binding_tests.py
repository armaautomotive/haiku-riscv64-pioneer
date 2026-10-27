#!/usr/bin/env python3
"""Actual native binding/retention with real zero+QPE1; OS/MMIO simulated."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
for path,name,digest in ((pathlib.Path(sys.argv[1]),'text.bin','39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'),
 (pathlib.Path(sys.argv[2]),'code.bin','c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1')):
 assert hashlib.sha256((path/name).read_bytes()).hexdigest()==digest
role=(kernel/'PioneerQ38MixedLiveNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==20
defines={key:'1' for key in parents};defines['PIONEER_DRIVER_BUILD_TAG']='0x51424c4e4f445256ULL'
defines.update({key:'pioneer_mixed_binding_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
binding=(kernel/'PioneerQ38MixedLiveNative.inc').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-native-bind-',dir='/private/tmp') as temporary:
 base=pathlib.Path(temporary)
 for name,marker in (('scope','PIONEER_Q38_EMBEDDING_NATIVE_SCOPE'),('unsafe','PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE'),
  ('mapping','PIONEER_Q38_EMBEDDING_NATIVE_MAPPING'),('factory','PIONEER_Q38_EMBEDDING_NATIVE_FACTORY'),
  ('end','PIONEER_Q38_EMBEDDING_NATIVE_END'),('retain','PIONEER_Q38_RETAIN_MEMORY')):
  begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert source.count(begin)==source.count(end)==1
  (base/('embedding_factory_'+name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
 first='bool PioneerSessionIO::MixedLiveMappingSourceScope()'
 start='[[maybe_unused]] static bool MixedLiveLearnedPages('
 end='[[maybe_unused]] static bool PrepareQ38MixedLiveNativeHeld('
 assert binding.count(first)==binding.count(start)==binding.count(end)==1
 (base/'mixed_live_factory_bind.inc').write_text(binding.split(first,1)[0]+'\n#endif\n'+start+binding.split(start,1)[1].split(end,1)[0])
 binary=base/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),'-I'+str(kernel),
  str(here/'pioneer_q38_mixed_live_native_binding_test.cpp'),str(here/'pioneer_q38_embedding_execution_bridge.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:]],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
