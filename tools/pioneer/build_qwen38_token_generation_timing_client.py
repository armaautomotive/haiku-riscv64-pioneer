#!/usr/bin/env python3
"""Build separate host-wall-timed generation USER diagnostic;NEVER run hardware."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA='262692ebbc03aa9b6cd1e6e036a2f50e820840c79b025bd90142527ec8ec2712'
BINARY_SHA='efb8d649a41b02d78130476508cde63b0d6a174e5aed66d8bc6c77f51a741e38'
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_generation_diagnostic_client.py'),str(build),str(out/'unchanged-generation')]
    control=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# The old b15 generation and 1a061 hidden literal controls have just passed.
cxx,flags,includes,libraries,release,tool=(control[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'generation-timing-client.o'
subprocess.run([cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
                '-DPIONEER_Q38_TOKEN_GENERATION_HOST_TIMING=1',*includes,'-c',
                str(here/'pioneer_q38_token_generation_diagnostic.cpp'),'-o',str(obj)],check=True)
binary=out/'pioneer-q38-token-DEBUG75-GENERATION-TIMED-DIAGNOSTIC-ONLY'
subprocess.run([cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
                '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)],check=True)
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for required in ('PioneerQ38GenerationHostTiming','PioneerQ38TokenHiddenValidationSession','PioneerQ38PublicModelSource','ioctl'):
    assert required in symbols,required
for forbidden in ('PioneerQ38DiagnosticHostTiming','PioneerQ38TokenValidationSession','llama_','ggml_',
                  'OriginalSource','ReferenceProducts','PioneerSessionIO','PioneerStoreContext','Q38WorkControl',
                  'NativeQ38PeerInstructionModel','Q38NativePeerTestCpu','SyntheticIO','SyntheticSource'):
    assert forbidden not in symbols,forbidden
data=binary.read_bytes()
for required in (b'--generate-three-positions\0',b'--preflight-source-only\0',b'/bin/sha256sum\0',
                 b'DEBUG75_GENERATION_HOST_TIMING',b'HOST_WALL_ONLY_NOT_ACCELERATOR_CYCLES_OR_TOKEN_RATE',
                 b'source_to_device_open\0',b'upload_ioctl\0',b'startup_ioctl\0',b'cleanup_ioctl\0'):
    assert required in data,required
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows),rows
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    actual=hashlib.sha256(path.read_bytes()).hexdigest();assert actual==expected,path
    print(actual+' '+path.name)
print('PASS separate tag75 host-wall observation/actual USER clock and same fixed generation lifecycle/old b15+hidden literal controls unchanged/no CPU oracle+native+Work+MOCK imports/maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)))
print('BUILD+INSPECT ONLY;stderr phases MUST stay separate from stdout record log;NOT on-card cycles,steady decode TPS,physical numerics or a resident Work release')
