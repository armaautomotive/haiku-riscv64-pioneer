#!/usr/bin/env python3
"""Separate NEW native component stays closed to old IO/drivers/CPU/Work."""
import os
import pathlib
import re
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
directory,kernel=here.parent,here.parents[2]/'src/add-ons/kernel/drivers/misc'
flag='PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT'
valid={flag:'1','PIONEER_Q38_PROJECTION_EXECUTION_OWNER':'1','PIONEER_Q38_PROJECTION_RESULT_STASH':'1',
       'PIONEER_Q38_LEARNED_RMS_NATIVE':'1','PIONEER_Q38_EMBEDDING_NATIVE':'1',
       'PIONEER_Q38_DECODER_RUNTIME':'1','PIONEER_Q38_GDN_MUTABLE_STATE':'1'}
role='#include "'+str(kernel/'PioneerQ38ResidentLayerNativeRole.h')+'"\n'
clean={k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
cxx,count=os.environ.get('CXX','c++'),0


def check(text,definitions,expected,syntax=False):
    global count
    result=subprocess.run([cxx,'-std=c++17','-x','c++','-fsyntax-only' if syntax else '-E',
                           *['-D'+k+'='+v for k,v in definitions.items()],'-'],
                          input=text,text=True,capture_output=True,env=clean,timeout=30)
    assert (result.returncode==0)==expected,(definitions,result.stderr)
    count+=1
    return result


check(role,valid,True)
check(role,{**valid,'_KERNEL_MODE':'1'},True)
# A genuine QPS1 producer may compile its typed old export record;this
# does NOT admit old68 native scope/ownership or make this role a driver.
check(role,{**valid,'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':'1'},True)
for name in valid:
    for value in (None,'0','2','-1',''):
        definitions=dict(valid)
        if value is None:
            del definitions[name]
        else:
            definitions[name]=value
        check(role,definitions,False)
    check(role+'#undef '+name+'\n'+role,valid,False)
for name in sorted(set(re.findall(r'defined\(([A-Z][A-Z0-9_]*)\)',(kernel/'PioneerQ38ResidentLayerNativeRole.h').read_text()))-set(valid)):
    for value in ('0','1'):
        check(role,{**valid,name:value},False)
        check(role+'#define '+name+' '+value+'\n'+role,valid,False)
for name in ('Embedding','LearnedRms','Projection','ResidentDecoder','GdnInput','GdnRecurrent','GdnRendezvous','GdnOutput','NextProjection','NextGdnOutput'):
    kind='GDN_OUTPUT' if name=='GdnOutput' else 'GDN_RENDEZVOUS' if name=='GdnRendezvous' else 'GDN_RECURRENT' if name=='GdnRecurrent' else 'GDN_INPUT'
    cpu={'PIONEER_Q38_'+kind+'_VALIDATION_CLIENT':'1','PIONEER_Q38_CPU_VALIDATION':'1','PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE':'1'}
    if name=='NextGdnOutput':
        cpu=dict(PIONEER_Q38_NEXT_GDN_OUTPUT_VALIDATION_REFERENCE='1',PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1',PIONEER_Q38_CPU_VALIDATION='1')
    text='#include "'+str(directory/('PioneerQ38'+name+'ValidationReference.h'))+'"\n'
    check(text,cpu,True)
    for value in ('0','1'):
        check(text,{**cpu,flag:value},False)
        check(text+'#define '+flag+' '+value+'\n'+text,cpu,False)
session='#include "'+str(directory/'PioneerQ38GdnRendezvousValidationSession.h')+'"\n'
cpu=dict(PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT='1',PIONEER_Q38_CPU_VALIDATION='1',PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
check(session,cpu,True)
for value in ('0','1'):
    check(session,{**cpu,flag:value},False)
    check(session+'#define '+flag+' '+value+'\n'+session,cpu,False)
prefix=(kernel/'tenstorrent_pci_probe.cpp').read_text().split('#ifdef PIONEER_Q38_PROJECTION_VALIDATION_NATIVE',1)[0].replace('#include "tenstorrent_pci_probe.h"','')
check(prefix,{},True)
for value in ('0','1'):
    check(prefix,{flag:value},False)
    for tag in ('0x2026102100010012ULL','0x2026102500010065ULL','0x2026102500010068ULL'):
        check(prefix,{flag:value,'TT_PIONEER_DRIVER_RELOAD':'1','PIONEER_DRIVER_BUILD_TAG':tag},False)
    result=subprocess.run([cxx,'-std=c++17','-E','-D'+flag+'='+value,str(directory/'pioneer_q38_original_source.cpp')],
                          capture_output=True,text=True,env=clean,timeout=30)
    assert result.returncode!=0 and 'Complete resident layer IMAGE never contains CPU reference observers' in result.stderr
    count+=1
# Compile the REAL new journal against OLD memory IO:its old Scope/map/
# read/write/retain callbacks cannot satisfy the NEW component contract.
old_io='#define PIONEER_Q38_RESIDENT_OUTPUT_MEMORY_FIXTURE_ONLY\n#include "'+str(here/'pioneer_q38_resident_output_memory_test.cpp')+'"\n'
old_io+='#include "'+str(kernel/'PioneerQ38ResidentLayerMemory.h')+'"\n'
old_io+='bool rejected(PioneerQ38ResidentLayerMemory& memory,ResidentOutputMemoryFixture& f){PioneerQ38ResidentLayerMemoryResult result;return memory.PrepareHeld(f.io,f.dram,f.banks,*f.cohort,*f.plan,f.id,f.ledger.data(),unsigned(f.ledger.size()),result);}\n'
result=check(old_io,{},False,True)
assert 'ResidentLayerMemoryScope' in result.stderr and 'ResidentLayerRetainHostOwners' in result.stderr
with tempfile.TemporaryDirectory(prefix='pioneer-layer-native-gates-',dir='/private/tmp') as temporary:
    temp=pathlib.Path(temporary)
    for value in ('0','1','2','-1','','false','0\n1'):
        for name in ('banks_driver','banks_client','inference','local_image','ffn_graph_image','ffn_matrix_image','ffn_persistent_image','projection_graph_image','projection_matrix_image','projection_persistent_image'):
            argv=[str(temp/'NO_SDK'),*([str(temp/'NO_LIB')] if name=='inference' else []),str(temp/'NO_OUTPUT')]
            result=subprocess.run(['sh',str(directory/('build_qwen38_'+name+'.sh')),*argv],capture_output=True,text=True,
                                  env={**clean,flag:value},timeout=5)
            assert result.returncode==2 and not result.stdout and not result.stderr and not (temp/'NO_OUTPUT').exists()
            count+=1
print(f'PASS NEW native-component gates{count}:exact separate producer/runtime roles,defined0+late CPU/Work/old68/image/native aliases refused,OLD IO cannot instantiate NEW44MB journal,default tags12/65/68 and all legacy build entries inert;NO driver/native factory/launch/physical authority')
