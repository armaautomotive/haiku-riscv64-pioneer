#!/usr/bin/env python3
"""Build/inspect separate longer USER diagnostic. Never install/run hardware."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
OBJECT_SHA='d1caf8139df295fb8e7a97e3639e1073ca5319fd954293e4d1b33168f212ff24'
BINARY_SHA='05510422cb94afa60689d69c1523add442b38a0679182dee5da1986b2a650b13'
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_generation_timing_client.py'),str(build),str(out/'unchanged-short-client')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
cxx,flags,includes,libraries,release,tool=(control[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'decode-benchmark-client.o'
subprocess.run([cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
                '-DPIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT=1',*includes,'-c',
                str(here/'pioneer_q38_decode_benchmark.cpp'),'-o',str(obj)],check=True)
binary=out/'pioneer-q38-DECODE34-BENCHMARK-DIAGNOSTIC-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
                '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for required in ('DecodeBenchmarkTransport','PioneerQ38TokenHiddenValidationSession','PioneerQ38PublicModelSource','ioctl'):
    assert required in symbols,required
for forbidden in ('llama_','ggml_','OriginalSource','ReferenceProducts','PioneerSessionIO','PioneerStoreContext',
                  'Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','SyntheticSource'):
    assert forbidden not in symbols,forbidden
data=binary.read_bytes()
for required in (b'--generate-34-positions\0',b'--preflight-source-only\0',b'/bin/sha256sum\0',
                 b'NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_STEADY_TOKEN_RATE',b'HOST WALL ONLY;'):
    assert required in data,required
for forbidden in (b'--generate-three-positions\0',b'--capture-first-layer\0',b'--work\0',b'--cpu\0'):
    assert forbidden not in data,forbidden
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows),rows
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    actual=hashlib.sha256(path.read_bytes()).hexdigest()
    assert actual==expected,(path,actual)
    print(actual+' '+path.name)
print('PASS_FROZEN separate34-position USER diagnostic/old short+timed+hidden literal controls unchanged/'
      'same-open full SHA/source/real ioctl lifecycle/no CPU oracle+native+Work+MOCK imports/'
      'maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)))
print('NO install/run authority;34 positions include two prompt positions;host startup may overlap inference;NOT numerical/native/physical/resident-Work/steady-TPS qualification')
