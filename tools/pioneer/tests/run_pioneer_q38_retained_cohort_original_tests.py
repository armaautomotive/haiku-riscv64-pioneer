#!/usr/bin/env python3
"""Pinned retained-cohort first-layer original-byte instruction history ONLY.

No native driver,hardware,full model,STOP/END,Work or timing acceptance.
"""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

local_sources='--cohort-local-sources' in sys.argv[1:]
if local_sources:
    assert sys.argv.count('--cohort-local-sources')==1
    sys.argv.remove('--cohort-local-sources')
first_layer=len(sys.argv)==8 and sys.argv[5]=='--first-layer-only' and sys.argv[6]=='--observed-trace'
faults=len(sys.argv)==6 and sys.argv[5]=='--conv-transition-faults'
source_faults=local_sources and len(sys.argv)==6 and sys.argv[5]=='--cohort-source-faults'
cold_faults=local_sources and len(sys.argv)==6 and sys.argv[5]=='--cold-source-faults'
assert first_layer or faults or source_faults or cold_faults
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image,old,worker,model=map(pathlib.Path,sys.argv[1:5]);observed=pathlib.Path(sys.argv[7]) if first_layer else None
assert all(p.is_absolute() and not p.is_symlink() for p in (image,old,worker,model))
assert model.is_file() and (observed is None or observed.is_absolute() and not observed.exists() and not observed.is_symlink())
here=pathlib.Path(__file__).resolve().parent
def pins(script):
    return next(ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body
                if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
for directory,script in ((image,here.parent/('build_qwen38_cohort_local_sources_root.py' if local_sources else 'build_qwen38_retained_cohort_root.py')),
                         (old,here/'run_pioneer_q38_mixed_worker_tests.py'),
                         (worker,here.parent/'build_qwen38_precise_beta_worker_image.py')):
    expected=pins(script);assert expected
    for name,size,digest in expected:
        data=(directory/name).read_bytes()
        assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
for directory in (image,worker):
    assert subprocess.check_output([tool+'nm','-n',str(directory/'program.elf')],text=True)==(directory/'program.symbols').read_text()
intervals={}
for line in subprocess.check_output([tool+'objdump','-tC',str(image/'program.elf')],text=True).splitlines():
    fields=line.split(None,5)
    if len(fields)!=6 or fields[2]!='F':continue
    start,size=int(fields[0],16),int(fields[4],16)
    if not size:continue
    assert not (start|size)&3
    assert (0x40000<=start and start+size<=0x60000) or (0x64000<=start and start+size<=0x6c000)
    intervals.setdefault((start,size),fields[5])
end=0
for (start,size),name in sorted(intervals.items()):assert start>=end;end=start+size
flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
assert flags
with tempfile.TemporaryDirectory(prefix='pioneer-q38-retained-conv-original-',dir='/private/tmp') as temporary:
    out=pathlib.Path(temporary);binary=out/'test';bridge=out/'observed-prefix-data.o'
    profile=out/'profile.map'
    profile.write_text(''.join(f'{start:x} {size:x} {name}\n' for (start,size),name in sorted(intervals.items())))
    common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
            '-ffp-contract=off','-fsanitize=address,undefined']
    recorder=[]
    if first_layer:
        subprocess.run([*common,'-c',str(here/'pioneer_q38_eos_state_data_bridge.cpp'),'-o',str(bridge)],check=True)
        recorder=[str(bridge)]
    test_source=('pioneer_q38_token_original_flow_test.cpp' if first_layer else
                 'pioneer_q38_connected_cohort_sources_test.cpp' if source_faults else
                 'pioneer_q38_connected_cold_sources_test.cpp' if cold_faults else
                 'pioneer_q38_connected_retained_conv_test.cpp')
    entries=[]
    if source_faults:
        symbols=subprocess.check_output([tool+'nm','-nC',str(image/'program.elf')],text=True)
        found=re.findall(r'^([0-9a-f]+) [tTW] MixedRecurrentCohort::StageConv\(',symbols,re.M)
        assert len(found)==1
        entries=found
    subprocess.run([*common,*flags,'-DPIONEER_Q38_TEST_CANCELLABLE_ROOT=1',
                    '-DPIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION=1','-DPIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION=1',
                    *(['-DPIONEER_Q38_TEST_OBSERVED_TRACE=1'] if first_layer else []),
                    str(here/test_source),
                    str(here/'pioneer_q38_whole_original_bytes.cpp'),*recorder,'-o',str(binary)],check=True)
    # Only now create the observation destination. No synthetic learned
    # products,CPU references,processor histories or completion setters.
    if observed is not None:observed.mkdir(mode=0o700)
    subprocess.run([str(binary),str(image),str(old),str(worker),str(model),
                    *(['--first-layer-only','--observed-trace',str(observed)] if first_layer else entries)],check=True,
                   env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1',
                        'PIONEER_TEST_ROOT_PROFILE_MAP':str(profile)})
    if observed is not None:assert sorted(p.name for p in observed.iterdir())==['prefix-0.bin']
print(('PASS actual cohort SAME original27B embedding+layer0 instructions/real119 closed histories;strict posthoc numerical comparison still REQUIRED;'
       if first_layer else ('PASS actual original-byte four-matrix->Conv physical-source faults and unchanged parent control;' if source_faults else 'PASS actual original-byte cold remote source refusals and unchanged parent control;' if cold_faults else 'PASS actual retained-cohort original-byte Conv->GDN transition raw-CSR fault children and unchanged parent control;'))+
      'NOT native/whole-model/STOP/END/physical/Work/TPS permission')
