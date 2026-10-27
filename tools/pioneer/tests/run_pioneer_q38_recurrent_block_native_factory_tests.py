#!/usr/bin/env python3
"""Verbatim new native factories + actual zero/QPE1/QRB1/QMW1; simulated raw IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
original_model=None
control=len(sys.argv)==6 and sys.argv[5]=='--control'
readback=control or len(sys.argv)>=6 and sys.argv[5] in ('--fresh-readback-end','--readback-original-model')
if len(sys.argv)==7 and sys.argv[5] in ('--original-model','--readback-original-model'):
 original_model=pathlib.Path(sys.argv[6]);assert original_model.is_absolute() and original_model.stat().st_size==18973870528
 options=['--fresh-execute-end']
else:
 assert len(sys.argv) in (5,6) and (len(sys.argv)==5 or sys.argv[5] in ('--execute','--faults','--fresh-held-end','--fresh-execute-end','--fresh-readback-end','--control'))
 options=sys.argv[5:]
 if readback:options=['--fresh-execute-end']
fresh=bool(options) and options[0].startswith('--fresh-')
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
zero,embedding,root,worker=map(pathlib.Path,sys.argv[1:5])
for directory,name,size,digest in ((zero,'text.bin',None,'39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'),
 (embedding,'code.bin',None,'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'),
 (root,'boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 (root,'text.bin',52596,'6da8623de3970219ca10fa092adf0d4c08e7275f92f6eec436bb99c6f333da17'),
 (root,'conv.bin',7500,'16ff564dc406a72a918f7c67c40f6815a7430f6d02161a0be0b26ec56cde7f65'),
 (root,'program.elf',75184,'dfaee3da2e38471333b9bfef3d031ef06cffbe0b6ea29df5adae8391df662f2f')):
 data=(directory/name).read_bytes();assert (size is None or len(data)==size) and hashlib.sha256(data).hexdigest()==digest
tree=ast.parse((here/'run_pioneer_q38_mixed_worker_tests.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for name,size,digest in pins:
 data=(worker/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
role=(kernel/('PioneerQ38RecurrentBlockValidationRole.h' if control else 'PioneerQ38RecurrentBlockNativeRole.h')).read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==(22 if control else 21)
defines={key:'1' for key in parents};defines['PIONEER_DRIVER_BUILD_TAG']='0x2026102700010072ULL' if control else '0x5152424e4f445256ULL'
if not control:defines.update({key:'pioneer_recurrent_factory_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
if fresh:defines.update({'PIONEER_Q38_RECURRENT_BLOCK_END_FIXTURE':'1','PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE':'1','PIONEER_Q38_RECURRENT_BLOCK_FRESH_FINAL_TEST':'1'})
if readback:defines['PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK']='1'
if original_model:defines['PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST']='1'
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-qrb-native-factory-',dir='/private/tmp') as temporary:
 base=pathlib.Path(temporary)
 for name,marker in (('scope','PIONEER_Q38_EMBEDDING_NATIVE_SCOPE'),('unsafe','PIONEER_Q38_EMBEDDING_NATIVE_UNSAFE'),
  ('mapping','PIONEER_Q38_EMBEDDING_NATIVE_MAPPING'),('factory','PIONEER_Q38_EMBEDDING_NATIVE_FACTORY'),
  ('end','PIONEER_Q38_EMBEDDING_NATIVE_END'),('retain','PIONEER_Q38_RETAIN_MEMORY')):
  begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert source.count(begin)==source.count(end)==1
  (base/('embedding_factory_'+name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
 begin='static bool\nEndQ38DispatchOwnership(';end='// PIONEER_Q38_DISPATCH_RETAIN_END'
 assert source.count(begin)==source.count(end)==1
 (base/'projection_factory_complete_end.inc').write_text(begin+source.split(begin,1)[1].split(end,1)[0])
 def method(signature):
  assert source.count(signature)==1
  start=source.index(signature);at=source.index('{',start);depth=1;end=at+1
  while depth:
   depth+=(source[end]=='{')-(source[end]=='}');end+=1
  return source[start:end]+'\n'
 (base/'recurrent_block_zero_callbacks.inc').write_text(''.join(method(s) for s in (
  'bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()', 'bool PioneerSessionIO::NonRootHeldOwner()')))
 if control:
  begin='static status_t\nProbeControl(';assert source.count(begin)==1
  route=source.split(begin,1)[1].split('#ifdef TT_PIONEER_DRIVER_RELOAD',1)[0]
  assert 'Q38RecurrentBlockValidationControl(instance,operation,data,length)' in route and 'return B_NOT_ALLOWED;' in route
  (base/'native_recurrent_control_routing.inc').write_text(begin+route+'return B_NOT_ALLOWED;\n}\n')
 if fresh:
  begin,end='// PIONEER_Q38_CLOCK_SERVICE_BEGIN','// PIONEER_Q38_CLOCK_SERVICE_END'
  assert source.count(begin)==source.count(end)==1
  (base/'native_clock_service.inc').write_text(source.split(begin,1)[1].split(end,1)[0])
  begin,end='static bool\nCloseBulkScratch(','static bool\nFleetHeld('
  assert source.count(begin)==source.count(end)==1
  (base/'native_bulk_close.inc').write_text(begin+source.split(begin,1)[1].split(end,1)[0])
  def member(owner,signature):
   part=source.split('struct '+owner+' ',1)[1];start=part.index(signature);at=part.index('{',start);depth=1;end=at+1
   while depth:depth+=(part[end]=='{')-(part[end]=='}');end+=1
   return part[start:end]+'\n'
  for name,owner,signature in [('host','PioneerHostPageIO','int32 ReleaseHostPage()'),('output','PioneerOutputBufferIO','int32 ReleaseOutputBuffer()'),('wide','PioneerWideOutputIO','int32 ReleaseWideOutput()')]:
   (base/('native_release_'+name+'.inc')).write_text(member(owner,signature))
  (base/'native_session_unmap.inc').write_text(member('PioneerSessionIO','bool Unmap()'))
  for owner in ('Arc','Telemetry','Tensix','DramSamples','ComputeNoc','HostPath','HostDmaAudit'):
   (base/('native_audit_'+owner+'.inc')).write_text(member('Pioneer'+owner+'IO','bool AfterNodeId(uint32 id)'))
  for name,marker in (('probe_lock','PIONEER_PROBE_LOCK'),('call_scope','PIONEER_Q38_GDN_CALL_SCOPE')):
   begin,end='// '+marker+'_BEGIN','// '+marker+'_END';assert source.count(begin)==source.count(end)==1
   (base/('native_'+name+'.inc')).write_text(source.split(begin,1)[1].split(end,1)[0])
  begin='q38_dispatch_owned_restored:';end='// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_BEGIN'
  assert source.count(begin)==source.count(end)==1
  (base/'native_final_tail.inc').write_text('#ifdef TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH\n'+begin+source.split(begin,1)[1].split(end,1)[0])
  part=source.split('static void\nEndResidentSession(',1)[1].split(end,1)[0]
  prefix=part.split('bool clean=!sColdRecoveryRequired && !context->Unsafe();',1)[1].split('#ifdef TT_PIONEER_EXPERIMENTAL_Q38_COHORT',1)[0]
  assert 'goto q38_dispatch_owned_restored;' in prefix and 'CloseBulkScratch' in prefix and 'EndQ38DispatchOwnership' in prefix
  (base/'native_final_prefix.inc').write_text(prefix)
 common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
 transport=base/'transport.o';binary=base/'test'
 flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
 if readback:flags+=['-DPIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK=1']
 subprocess.run([*common,*flags,'-c',str(here/'pioneer_q38_recurrent_block_native_transport.cpp'),'-o',str(transport)],check=True)
 native_flags=[*['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),'-I'+str(kernel)]
 sources=[here/('pioneer_q38_recurrent_block_validation_control_test.cpp' if control else 'pioneer_q38_recurrent_block_native_factory_test.cpp'),here/'pioneer_q38_embedding_execution_bridge.cpp',
  *([here/'pioneer_q38_mixed_live_native_clock.cpp',here/'pioneer_q38_mixed_live_final_audit.cpp'] if fresh else [])]
 if original_model:
  # Original file/source coordinator MUST NOT see native or operand/image
  # flags. Only the neutral original-BYTES virtual interface crosses TUs.
  objects=[transport]
  for path,flags in [*((p,native_flags) for p in sources),(here/'pioneer_q38_recurrent_block_original_native_test.cpp',[]),
   (here.parent/'pioneer_q38_recurrent_block_original_source.cpp',[])]:
   obj=base/(path.stem+'.o');subprocess.run([*common,*flags,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
  subprocess.run([*common,*map(str,objects),'-o',str(binary)],check=True)
  run_args=[str(original_model),*sys.argv[1:5]]
 else:
  subprocess.run([*common,*native_flags,*map(str,sources),str(transport),'-o',str(binary)],check=True)
  run_args=[*sys.argv[1:5],*([] if control else options)]
 subprocess.run([str(binary),*run_args],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
