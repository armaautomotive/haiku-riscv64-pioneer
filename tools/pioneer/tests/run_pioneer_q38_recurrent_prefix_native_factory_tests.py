#!/usr/bin/env python3
"""Actual prefix native scopes+zero/QPE1/3layer execution; SIMULATED raw IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile
original_model=None;readback=False
if len(sys.argv)==7 and sys.argv[5] in ('--original-model','--original-model-readback'):
 original_model=pathlib.Path(sys.argv[6]);assert original_model.is_absolute() and original_model.stat().st_size==18973870528
 readback=sys.argv[5]=='--original-model-readback';options=['--fresh-readback-end' if readback else '--fresh-execute-end']
else:
 assert len(sys.argv) in (5,6) and (len(sys.argv)==5 or sys.argv[5] in ('--execute','--faults','--fresh-held-end','--fresh-execute-end','--fresh-readback-end','--control-preflight','--control-execute','--control-routing-preflight','--control-routing-execute'))
 options=sys.argv[5:]
 readback=options==['--fresh-readback-end']
control=bool(options) and options[0].startswith('--control-')
routed_control=control and options[0].startswith('--control-routing-')
fresh=control or bool(options) and options[0].startswith('--fresh-')
readback=readback or control
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
zero,embedding,root,worker=map(pathlib.Path,sys.argv[1:5])
for directory,name,digest in ((zero,'text.bin','39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'),
 (embedding,'code.bin','c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1')):
 assert hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest
for directory,runner in ((root,'run_pioneer_q38_recurrent_prefix_root_tests.py'),(worker,'run_pioneer_q38_mixed_worker_tests.py')):
 tree=ast.parse((here/runner).read_text())
 pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
 for name,size,digest in pins:
  data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
role=(kernel/'PioneerQ38RecurrentPrefixNativeRole.h').read_text()
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role));assert len(parents)==21
defines={key:'1' for key in parents};defines['PIONEER_DRIVER_BUILD_TAG']='0x5152504e4f445256ULL'
defines.update({key:'pioneer_prefix_factory_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
if fresh:defines.update({'PIONEER_Q38_RECURRENT_PREFIX_END_FIXTURE':'1','PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE':'1','PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST':'1'})
if readback:defines['PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_READBACK']='1'
if control:defines['PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_CONTROL_COMPILE_ONLY']='1'
if routed_control:defines['PIONEER_Q38_RECURRENT_PREFIX_ROUTED_CONTROL_TEST']='1'
if original_model:defines['PIONEER_Q38_RECURRENT_PREFIX_ORIGINAL_NATIVE_TEST']='1'
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
with tempfile.TemporaryDirectory(prefix='pioneer-qpx-native-factory-',dir='/private/tmp') as temporary:
 base=pathlib.Path(temporary)
 # Only TEST physical endpoint names are adapted. Actual production scopes,
 # factories, private leases, phase transitions and controllers stay verbatim.
 fixture=(here/'pioneer_q38_embedding_native_factory_test.cpp').read_text()
 fixture=fixture.replace('RecurrentBlock','RecurrentPrefix').replace('RECURRENT_BLOCK','RECURRENT_PREFIX')\
  .replace('recurrent_block','recurrent_prefix').replace('recurrentBlock','recurrentPrefix')
 def include_path(match):
  candidate=(here/match[1]).resolve()
  return '#include "'+str(candidate)+'"' if candidate.exists() else match[0]
 (base/'prefix_embedding_factory_fixture.inc').write_text(re.sub(r'#include "([^"]+)"',include_path,fixture))
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
  while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
  return source[start:end]+'\n'
 (base/'recurrent_prefix_zero_callbacks.inc').write_text(''.join(method(s) for s in (
  'bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()', 'bool PioneerSessionIO::NonRootHeldOwner()')))
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
  if control:
   snapshot=member('PioneerStoreContext','void Snapshot(tt_resident_session_result& r)')
   assert snapshot.startswith('void Snapshot(')
   (base/'native_prefix_snapshot.inc').write_text(snapshot.replace('void Snapshot(','void PioneerStoreContext::Snapshot(',1))
   protocol=(kernel/'tenstorrent_pci_probe.h').read_text()
   constants=protocol.split('#define TT_PIONEER_CAP_NATIVE_Q8_UNPACK ',1)[1].split('\n\nstruct tt_arc_query_support',1)[0]
   assert '#define TT_PIONEER_RESIDENT_CAPABILITIES ' in constants
   (base/'native_prefix_snapshot_protocol.inc').write_text('#define TT_PIONEER_CAP_NATIVE_Q8_UNPACK '+constants+'\n')
   if routed_control:
    start=source.index('static status_t\nProbeControl(');branch=source.index('#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT',start)
    head=source[start:branch];assert 'geteuid() != 0' in head and 'PioneerQ38MixedLiveNativeRouting.inc' in head
    depth=0;selected=[]
    for line in source[branch:].splitlines(keepends=True):
     text=line.lstrip()
     if text.startswith(('#if ','#if\t','#ifdef ','#ifndef ')):depth+=1
     selected.append(line)
     if text.startswith('#endif'):
      depth-=1
      if not depth:break
    assert depth==0 and 'Q38RecurrentPrefixValidationControl(instance,operation,data,length)' in ''.join(selected)
    (base/'native_prefix_public_routing.inc').write_text(head+''.join(selected)+'\n}\n')
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
 flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
 transport=base/'transport.o';binary=base/'test'
 subprocess.run([*common,*flags,'-c',str(here/'pioneer_q38_recurrent_prefix_native_transport.cpp'),'-o',str(transport)],check=True)
 native_flags=[*['-D'+k+'='+v for k,v in sorted(defines.items())],'-I'+str(base),'-I'+str(kernel)]
 sources=[here/('pioneer_q38_recurrent_prefix_validation_control_test.cpp' if control else 'pioneer_q38_recurrent_prefix_native_factory_test.cpp'),here/'pioneer_q38_embedding_execution_bridge.cpp',
  *([here/'pioneer_q38_mixed_live_native_clock.cpp',here/'pioneer_q38_mixed_live_final_audit.cpp'] if fresh else [])]
 if original_model:
  # Private CPU reference/source/coordinator MUST NOT see native or
  # operand/image/Work flags. Only neutral raw bytes and post-END outputs
  # cross TUs; inspect native+operand objects before linking the oracle.
  objects=[transport];native_objects=[]
  for path,selection in [*((p,native_flags) for p in sources),
   (here/'pioneer_q38_recurrent_prefix_original_native_test.cpp',[]),(here.parent/'pioneer_q38_recurrent_prefix_original_source.cpp',[])]:
   obj=base/(path.stem+'.o');subprocess.run([*common,*selection,'-c',str(path),'-o',str(obj)],check=True);objects.append(obj)
   if selection:native_objects.append(obj)
  for obj in [transport,*native_objects]:
   table=subprocess.check_output(['nm','-C',str(obj)],text=True)
   for forbidden in ('OpenPioneerQ38','ReferenceFingerprint','PioneerQ38RecurrentPrefixOriginalSource','PioneerRecurrentPrefixReference'):
    assert forbidden not in table,(obj,forbidden,'private oracle must not enter native/operand objects')
  subprocess.run([*common,*map(str,objects),'-o',str(binary)],check=True)
  run_args=[str(original_model),*sys.argv[1:5]]
 else:
  subprocess.run([*common,*native_flags,*map(str,sources),str(transport),'-o',str(binary)],check=True)
  run_args=[*sys.argv[1:5],*options]
 subprocess.run([str(binary),*run_args],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
