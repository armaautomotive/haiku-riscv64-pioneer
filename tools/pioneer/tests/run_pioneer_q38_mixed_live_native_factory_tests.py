#!/usr/bin/env python3
"""Actual new native factories/held journal/complete END; simulated byte IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
assert len(sys.argv)>=7
options=sys.argv[7:]
assert len(set(options))==len(options) and set(options)<={'--execute','--faults','--sigmoid-candidate','--validation-readback','--clock-end','--final-end'}
assert len(set(options)&{'--execute','--faults','--validation-readback','--clock-end','--final-end'})<=1
final_end='--final-end' in options
clock_end='--clock-end' in options or final_end
readback='--validation-readback' in options or final_end
sigmoid='--sigmoid-candidate' in options or readback
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
for path,name,digest in ((pathlib.Path(sys.argv[1]),'text.bin','39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'),
 (pathlib.Path(sys.argv[2]),'code.bin','c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'),
 (pathlib.Path(sys.argv[3]),'boot.bin','32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 (pathlib.Path(sys.argv[3]),'code.bin','668a72ed92b42044d6fa8263bba8c63ff19ce31b1533b67e4a4d9799b9a2047e' if sigmoid else '50009c46fcdefb427dc1b31b68cc9b2403b4ba8e06b377ebd66a88bf7ce254c8'),
 (pathlib.Path(sys.argv[5]),'code.bin','ff39e984f76fe0d0591e9a48ea1137a231ecdaf2390b14223a8a8922b8e2488a'),
 (pathlib.Path(sys.argv[6]),'code.bin','e33cb65be5c53d77f486ee7ea2371a2ffc78638e5ed72bfa8b2fe2bf8cf19736')):
 assert hashlib.sha256((path/name).read_bytes()).hexdigest()==digest
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for name,size,digest in pins:
 data=(pathlib.Path(sys.argv[4])/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
role=(kernel/'PioneerQ38MixedLiveNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==20
defines={key:'1' for key in parents};defines['PIONEER_DRIVER_BUILD_TAG']='0x51424c4e4f445256ULL'
defines['PIONEER_Q38_MIXED_LIVE_NATIVE_FACTORY_FIXTURE']='1'
program_flags=['-DPIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS=1'] if sigmoid else []
if sigmoid:defines['PIONEER_Q38_MIXED_LIVE_SIGMOID_PROGRAMS']='1'
if readback:
 defines['PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK']='1'
 program_flags+=['-DPIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK=1']
if clock_end:defines['PIONEER_Q38_MIXED_LIVE_END_FIXTURE']='1'
if final_end:defines['PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE']='1'
defines.update({key:'pioneer_mixed_factory_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-native-factory-',dir='/private/tmp') as temporary:
 base=pathlib.Path(temporary)
 for name,marker in (('scope','PIONEER_Q38_EMBEDDING_NATIVE_SCOPE'),('unsafe','PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE'),
  ('mapping','PIONEER_Q38_EMBEDDING_NATIVE_MAPPING'),('factory','PIONEER_Q38_EMBEDDING_NATIVE_FACTORY'),
  ('end','PIONEER_Q38_EMBEDDING_NATIVE_END'),('retain','PIONEER_Q38_RETAIN_MEMORY')):
  begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert source.count(begin)==source.count(end)==1
  (base/('embedding_factory_'+name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
 begin='static bool\nEndQ38DispatchOwnership('
 end='// PIONEER_Q38_DISPATCH_RETAIN_END'
 assert source.count(begin)==source.count(end)==1
 (base/'projection_factory_complete_end.inc').write_text(begin+source.split(begin,1)[1].split(end,1)[0])
 if clock_end:
  begin,end='// PIONEER_Q38_CLOCK_SERVICE_BEGIN','// PIONEER_Q38_CLOCK_SERVICE_END'
  assert source.count(begin)==source.count(end)==1
  (base/'native_clock_service.inc').write_text(source.split(begin,1)[1].split(end,1)[0])
  begin,end='static bool\nCloseBulkScratch(','static bool\nFleetHeld('
  assert source.count(begin)==source.count(end)==1
  (base/'native_bulk_close.inc').write_text(begin+source.split(begin,1)[1].split(end,1)[0])
 if final_end:
  def method(class_name,signature):
   part=source.split('struct '+class_name+' ',1)[1];start=part.index(signature);at=part.index('{',start);depth=1;end=at+1
   while depth:
    depth+=(part[end]=='{')-(part[end]=='}');end+=1
   return part[start:end]+'\n'
  for name,owner,signature in [('host','PioneerHostPageIO','int32 ReleaseHostPage()'),('output','PioneerOutputBufferIO','int32 ReleaseOutputBuffer()'),('wide','PioneerWideOutputIO','int32 ReleaseWideOutput()')]:
   (base/('native_release_'+name+'.inc')).write_text(method(owner,signature))
  (base/'native_session_unmap.inc').write_text(method('PioneerSessionIO','bool Unmap()'))
  # VERBATIM actual owned-path footer. Prefix closure/clock state is
  # provided by the genuine factories,never a manufactured ready object.
  begin='q38_dispatch_owned_restored:';end='// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_BEGIN'
  assert source.count(begin)==source.count(end)==1
  (base/'native_final_tail.inc').write_text('#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH\n'+begin+source.split(begin,1)[1].split(end,1)[0])
  part=source.split('static void\nEndResidentSession(',1)[1].split(end,1)[0]
  prefix=part.split('bool clean=!sColdRecoveryRequired && !context->Unsafe();',1)[1].split('#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT',1)[0]
  assert 'goto q38_dispatch_owned_restored;' in prefix and 'CloseBulkScratch' in prefix and 'EndQ38DispatchOwnership' in prefix
  (base/'native_final_prefix.inc').write_text(prefix)
 binary=base/'test'
 # Transport is a separate TU: only RAW physical endpoints cross it. The
 # actual native role, source gates, private phases and factories stay here.
 common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
 transport=base/'transport.o'
 flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
 subprocess.run([*common,*flags,*program_flags,'-c',str(here/'pioneer_q38_mixed_live_native_transport.cpp'),'-o',str(transport)],check=True)
 subprocess.run([*common,
  *['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),'-I'+str(kernel),
  str(here/'pioneer_q38_mixed_live_native_factory_test.cpp'),str(here/'pioneer_q38_embedding_execution_bridge.cpp'),
  *([str(here/'pioneer_q38_mixed_live_native_clock.cpp')] if clock_end else []),
  str(transport),'-o',str(binary)],check=True)
 subprocess.run([str(binary),*sys.argv[1:7],*[o for o in options if o!='--sigmoid-candidate']],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
