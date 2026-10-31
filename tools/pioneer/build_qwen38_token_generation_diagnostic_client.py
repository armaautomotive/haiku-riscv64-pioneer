#!/usr/bin/env python3
"""Build the fixed tag75 generation diagnostic;NEVER deploy or run hardware."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA='785b914818a0ac8b9f566600fb85493cc3dcf348018357e8fbe4ac1c3dfdfbe2'
BINARY_SHA='b15da1871edf8f1c8acb59ab9b3978e3ace43b6d8bb4c3d8fd50ee240114d779'
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:])
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_hidden_validation_client.py'),str(build),str(out/'unchanged-hidden')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
for path,sha in ((control['obj'],'33cffe8edd7235e5b212a44937309dcde6d758e9bd09b5e30c4b35b6132c992c'),
                 (control['binary'],'1a061f7ab0eb5001c04a73c1334ed3b519a29781157ea4e46f533e3908fe0536')):
    assert hashlib.sha256(path.read_bytes()).hexdigest()==sha,('unchanged hidden client',path)
cxx,flags,includes,libraries,release,tool=(control[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'generation-client.o'
subprocess.run([cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',*includes,
                '-c',str(here/'pioneer_q38_token_generation_diagnostic.cpp'),'-o',str(obj)],check=True)
binary=out/'pioneer-q38-token-DEBUG75-GENERATION-DIAGNOSTIC-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
                '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
undefined=subprocess.check_output([str(tool)+'nm','-u',str(binary)],text=True)
for required in ('PioneerQ38TokenHiddenValidationSession','PioneerQ38PublicModelSource','PioneerQwen38PackedPlan','ioctl'):
    assert required in symbols,required
for forbidden in ('PioneerQ38TokenValidationSession','PioneerQ38TokenBoundaryValidationSession','llama_',
                  'ggml_','ReferenceProducts','OriginalSource','PioneerSessionIO','PioneerStoreContext',
                  'Q38WorkControl','NativeQ38PeerInstructionModel','Q38NativePeerTestCpu','SyntheticIO','SyntheticSource'):
    assert forbidden not in symbols and forbidden not in undefined,forbidden
data=binary.read_bytes()
for required in (b'/bin/sha256sum\0',b'--generate-three-positions\0',b'--preflight-source-only\0',
                 b'NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_TOKEN_RATE',
                 b'DATA_CONSISTENCY_ONLY_EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_STEADY_TOKEN_RATE'):
    assert required in data,required
assert b'--capture-first-layer\0' not in data and b'--capture-layer-boundary\0' not in data
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows),rows
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    actual=hashlib.sha256(path.read_bytes()).hexdigest();assert actual==expected,path
    print(actual+' '+path.name)
print('PASS fixed tag75 three-position USER diagnostic/old hidden object+binary LITERAL pins unchanged/actual same-open source uploader+normal terminal+END/192-word DATA publication/no CPU oracle+native+Work+MOCK imports/maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)))
print('BUILD+INSPECT ONLY;NO install/run,physical correctness,resident multi-request,Assistant or steady-state TPS qualification')
