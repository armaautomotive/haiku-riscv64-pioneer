#!/usr/bin/env python3
"""Actual emitted publication caller; explicitly mocked seals and NoC only."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
qgwf=len(sys.argv)>1 and sys.argv[1]=='--qgwf'
if qgwf:sys.argv[1]='--qgwd'
qgwd=len(sys.argv)>1 and sys.argv[1]=='--qgwd'
if qgwd:
    del sys.argv[1]
assert len(sys.argv) in ((3,) if qgwd else (3,5))
old,new=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert old.is_absolute() and new.is_absolute()
pins=((old,'430b89b2fb734bfb66bbe96cc1e8581fcd6476d263a2578b8e869936df4bc1c7'),
      (new,'5585e908bf3326c5410d0998c81ed161c04ab754412cc60891b8eb1f197a58fe'))
if qgwd:
    pins=((old,'2cb7e5f27f12255bd63dbc70b37df2fac1a532ed3c6f9927cde00ff71fca9d95'),
          (new,'a569498cca9175e4dbd13cf2f1f3dccb6596cf115b2d9b765a016cf9677a51f2'))
if qgwf:
    pins=((old,'5257f8da3b692ae964a0f29e7d895d0274985c113a8137653c53c655b34d8454'),
          (new,'e50ed27fba25611700a3a93b569a37145ad1ea400718ec7707326198ed560665'))
if len(sys.argv)==5:
    old_fault,new_fault=map(pathlib.Path,sys.argv[3:]);assert old_fault.is_absolute() and new_fault.is_absolute()
    pins+=((old_fault,'d0760386ea3d00616bfbf8bf53b2cff900701eef9491f6dc8efdc16163e422a6'),
           (new_fault,'9515589d211cb9d9b2db4be108ccad5752996ec07a5354f0181775f6a6888862'))
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
source=(here.parent/'blackhole_qwen38_token_root.cpp').read_text()
begin=source.index('struct Private {');end=source.index('static volatile uint32_t* Words(',begin)
private=source[begin:end]
assert 'Position=16,Token=17,Committed=18,Slot=19' in private and 'PrivateBytes' not in private
with tempfile.TemporaryDirectory(prefix='pioneer-q38-publish-instructions-',dir='/private/tmp') as name:
    temporary=pathlib.Path(name);binary=temporary/'test'
    (temporary/'actual_token_publication_private.inc').write_text(private)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',*flags,'-I'+str(temporary),
                    str(here/'pioneer_q38_token_publication_instruction_test.cpp'),'-o',str(binary)],check=True)
    for directory,digest in pins:
        assert hashlib.sha256((directory/'program.elf').read_bytes()).hexdigest()==digest
        # Inspect EXACT linked sections, not caller-supplied raw bytes.
        for section in ('text','conv','terminal'):
            path=temporary/(section+'.bin')
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(directory/'program.elf'),str(path)],check=True)
            assert path.read_bytes()==(directory/(section+'.bin')).read_bytes(),section
        table=subprocess.check_output([tool+'nm','-nC',str(directory/'program.elf')],text=True)
        entries=[]
        for function in ('PublishToken','Packet','HeadSeal','InputSeal','FeedbackSeal'):
            matches=re.findall(r'^([0-9a-f]+) [tTW] GlobalChain::'+function+r'\(',table,re.M)
            assert len(matches)==1,(function,matches)
            entries.append(matches[0])
        selected=directory==new or len(sys.argv)==5 and directory==new_fault
        assert ('GlobalChain::PublishFrontier(' in table)==selected
        subprocess.run([str(binary),str(directory),*entries,str(int(selected)),'--mocked-dependencies'],check=True,
                       env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
print('PASS unchanged-control and new-frontier ACTUAL caller instructions; exact linked sections; dependencies EXPLICITLY MOCKED; NOT full publication seals/model/native/hardware/clock/TPS qualification')
