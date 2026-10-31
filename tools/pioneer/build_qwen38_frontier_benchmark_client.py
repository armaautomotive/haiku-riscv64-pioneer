#!/usr/bin/env python3
"""Compile/inspect separately gated host client; NEVER install or run it."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4;assert not inspect or sys.argv[3]=='--inspect-candidate'
OBJECT_SHA='884f5e43f11de24d6264ab537e99344ff62fe6b9add64ce6daccea816b1d0b5f'
BINARY_SHA='4b1dbee63e537b39e5212a9180670139a2a9ba46ef0b3cbeb66a7d5226b7f048'
assert inspect or OBJECT_SHA and BINARY_SHA,'candidate not independently frozen'
build,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_decode_benchmark_client.py'),str(build),str(out/'unchanged-decode-client')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
# ALL old hidden/short/timed/decode literal controls reproduce FIRST.
cxx,flags,includes,libraries,release,tool=(old[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'frontier-benchmark-client.o'
subprocess.run([cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
                '-DPIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT=1','-DPIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT=1',
                *includes,'-c',str(here/'pioneer_q38_frontier_benchmark.cpp'),'-o',str(obj)],check=True)
binary=out/'pioneer-q38-FRONTIER34-BENCHMARK-DIAGNOSTIC-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
                '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for required in ('FrontierBenchmarkTransport','PioneerQ38TokenHiddenValidationSession','PioneerQ38PublicModelSource','ioctl'):
    assert required in symbols,required
for forbidden in ('llama_','ggml_','OriginalSource','ReferenceProducts','PioneerSessionIO','PioneerStoreContext',
                  'Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','SyntheticSource'):
    assert forbidden not in symbols,forbidden
data=binary.read_bytes()
for required in (b'--benchmark-34-positions\0',b'--preflight-source-only\0',b'/bin/sha256sum\0',
                 b'Q38_FRONTIER_OBSERVED_COUNT_RATE_BOUNDS',b'NUMERICAL_VALIDATION_REQUIRED',
                 b'POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF'):
    assert required in data,required
for forbidden in (b'--generate-three-positions\0',b'--generate-34-positions\0',b'--capture-first-layer\0',b'--work\0',b'--cpu\0'):
    assert forbidden not in data,forbidden
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows),rows
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name)
print('PASS_BUILD_ONLY new6607+6608 SAME-source+FD counter client/old05510422+allshort controls unchanged/'
      'fullGGUF SHA BEFORE Open/noCPU oracle+native+Work+MOCK imports/maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)))
print('No install/run grant;timing published only after wholeFinish+Close+34 validated records/finalcounter34;NOT model quality or measured TPS proof')
