#!/usr/bin/env python3
"""Closed GPU-producer handoff roles, no native/CPU/Work entry promotion."""
import os
import pathlib
import re
import subprocess
import tempfile

here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
flag = 'PIONEER_Q38_RESIDENT_OUTPUT_OWNER'
valid = {flag:'1', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER':'1', 'PIONEER_Q38_PROJECTION_RESULT_STASH':'1',
         'PIONEER_Q38_LEARNED_RMS_NATIVE':'1', 'PIONEER_Q38_EMBEDDING_NATIVE':'1'}
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cxx, count = os.environ.get('CXX', 'c++'), 0


def check(source, flags, expected, message=None):
    global count
    r = subprocess.run([cxx, '-std=c++17', '-E', '-x', 'c++', *['-D'+k+'='+v for k,v in flags.items()], '-'],
                       input=source, text=True, capture_output=True, env=clean, timeout=10)
    assert (r.returncode == 0) == expected, (flags, r.stderr)
    if message:
        assert message in r.stderr
    count += 1


role = '#include "'+str(kernel/'PioneerQ38ResidentOutputOwnerRole.h')+'"\n'
check(role, valid, True)
check('#include "'+str(kernel/'PioneerQ38ResidentProjectionInput.h')+'"\n', valid, True)
for key in valid:
    for value in (None, '', '0', '2', '-1'):
        changed = dict(valid)
        if value is None:
            del changed[key]
        else:
            changed[key] = value
        check(role, changed, False)
    check(role+'#undef '+key+'\n'+role, valid, False)
for key in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)', (kernel/'PioneerQ38ResidentOutputOwnerRole.h').read_text()))-set(valid)):
    for value in ('0', '1'):
        check(role, {**valid,key:value}, False)
        check(role+'#define '+key+' '+value+'\n'+role, valid, False)
parent = '#include "'+str(kernel/'PioneerQ38ProjectionGraphOwner.h')+'"\n'
base = {**valid, 'PIONEER_Q38_GDN_MUTABLE_STATE':''}
legacy = {k:v for k,v in base.items() if k != flag}
check(parent, base, True)
check(parent, legacy, True)
controller = '#include "'+str(kernel/'PioneerQ38ResidentOutputOwner.h')+'"\n'
runtime = {**base,'PIONEER_Q38_DECODER_RUNTIME':'1'}
check(controller,runtime,True)
for key in ('PIONEER_Q38_DECODER_RUNTIME','PIONEER_Q38_GDN_MUTABLE_STATE'):
    missing = dict(runtime);del missing[key]
    check(controller,missing,False,'complete genuine joint runtime reservation')
    check(controller+'#undef '+key+'\n'+controller,runtime,False,'complete genuine joint runtime reservation')
for value in ('','0','2','-1'):
    check(controller,{**runtime,'PIONEER_Q38_DECODER_RUNTIME':value},False)
for key in ('PIONEER_Q38_RESIDENT_DECODER_OWNER','PIONEER_Q38_RESIDENT_DECODER_NATIVE','PIONEER_Q38_GDN_RECURRENT_NATIVE','PIONEER_Q38_GDN_INPUT_OWNER'):
    for value in ('0','1'):
        check(controller,{**runtime,key:value},False,'NOT an old per-worker decoder alias')
        check(controller+'#define '+key+' '+value+'\n'+controller,runtime,False,'NOT an old per-worker decoder alias')
check(parent+'#define '+flag+' 1\n'+parent, legacy, False, 'cannot be selected after legacy inclusion')
check(parent+'#undef '+flag+'\n'+parent, base, False, 'layout cannot change after inclusion')
for value in ('0', '2', '-1'):
    check(parent+'#undef '+flag+'\n#define '+flag+' '+value+'\n'+parent, base, False)
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
prefix = source.split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',1)[0].replace('#include "tenstorrent_pci_probe.h"','')
check(prefix, {}, True)
for value in ('0','1'):
    check(prefix, {flag:value}, False, 'Resident-output owner is OFFLINE')
    check(prefix, {**valid,flag:value}, False, 'Resident-output owner is OFFLINE')
for name in ('Embedding','LearnedRms','Projection','ResidentDecoder','GdnInput','GdnRecurrent','GdnRendezvous','GdnOutput','NextProjection','NextGdnOutput'):
    kind = 'GDN_OUTPUT' if name=='GdnOutput' else 'GDN_RENDEZVOUS' if name=='GdnRendezvous' else 'GDN_RECURRENT' if name=='GdnRecurrent' else 'GDN_INPUT'
    cpu = {'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT':'1','PIONEER_Q38_CPU_VALIDATION':'1','PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    if name=='NextGdnOutput':
        cpu = dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1',PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1',PIONEER_Q38_CPU_VALIDATION='1')
    text = '#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text,cpu,True)
    for value in ('0','1'):
        check(text,{**cpu,flag:value},False)
        check(text+'#define '+flag+' '+value+'\n'+text,cpu,False)
session = '#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
cpu = dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1',PIONEER_Q38_CPU_VALIDATION='1',PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
for value in ('0','1'):
    check(session,{**cpu,flag:value},False)
    check(session+'#define '+flag+' '+value+'\n'+session,cpu,False)
    r = subprocess.run([cxx,'-std=c++17','-E','-D'+flag+'='+value,str(here/'pioneer_q38_decoder_original_source.cpp')],capture_output=True,text=True,env=clean,timeout=30)
    assert r.returncode != 0 and 'Original-model observer remains separate' in r.stderr
    count += 1
image = '#include "'+str(directory/'PioneerQ38ResidentOperatorImageRole.h')+'"\n'
image_flags = dict(PIONEER_Q38_RESIDENT_OPERATOR_IMAGE='1',PIONEER_Q38_FFN_PERSISTENT='1',PIONEER_Q38_PROJECTION_PERSISTENT='1')
check(image,image_flags,True)
for value in ('0','1'):
    check(image,{**image_flags,flag:value},False)
    check(image+'#define '+flag+' '+value+'\n'+image,image_flags,False)
with tempfile.TemporaryDirectory(prefix='pioneer-resident-input-gates-',dir='/private/tmp') as temporary:
    root = pathlib.Path(temporary)
    for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
        for value in ('0','1','2','-1','','false','0\n1'):
            args = [str(root/'NO_SDK'), *([str(root/'NO_LIB')] if name=='inference' else []), str(root/'NO_OUTPUT')]
            r = subprocess.run(['sh',str(directory/('build_qwen38_'+name+'.sh')),*args],capture_output=True,text=True,env={**clean,flag:value},timeout=5)
            assert r.returncode==2 and not r.stdout and not r.stderr and not (root/'NO_OUTPUT').exists()
            count += 1
print(f'PASS resident projection input gates{count}/exact genuine parents/complete controller runtime reservation/defined0+late old-owner+CPU+DEBUG+Work+image aliases/closed class-layout changes/10CPU observers/session/original model/10builders;existing kernel explicitly OFFLINE,NO native selector or IO grant')
