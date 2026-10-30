#!/usr/bin/env python3
"""Unfrozen inspect-only BUILD refusals must precede all output creation."""
import os
import pathlib
import subprocess
import sys
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent.parent
sdk=pathlib.Path('/Volumes/HaikuBuildLocal/generated.riscv64')
common=['--f32-hidden','--unroll-dense-seals','--boundary-seals','--unroll-quiet',
        '--worker-local-source-guards','--precise-activation','--single-closed-audit',
        '--worker-sealed-admission','--pushed-completion']
candidate='--unroll-packet-seals';eight='--unroll-packet-seals-eight';refusals=0
with tempfile.TemporaryDirectory(prefix='pioneer-packet-unroll-cli-',dir='/private/tmp') as temporary:
    root=pathlib.Path(temporary);sentinel=root/'keep';sentinel.write_bytes(b'unchanged\n')
    for script,parent in (('build_qwen38_token_root.py',True),('build_qwen38_global_token_components.py',False)):
        required=(['--cancel-boundary'] if parent else [])+common
        invalid=[[candidate],[candidate,'--inspect-candidate'],required+[candidate],
                 required+['--inspect-candidate',candidate,candidate],
                 required+['--inspect-candidate',candidate,'--unknown-packet-option']]
        invalid += [[value for value in required if value!=missing]+['--inspect-candidate',candidate] for missing in required]
        eight_required=required+[candidate,'--inspect-candidate']
        invalid += [[eight],eight_required+[eight,eight],required+[candidate,eight]]
        invalid += [[value for value in eight_required if value!=missing]+[eight] for missing in eight_required]
        for options in invalid:
            output=root/('refused-'+str(refusals))
            before=sorted(path.relative_to(root) for path in root.rglob('*'))
            result=subprocess.run([sys.executable,str(here/script),str(sdk),str(output),*options],
                                  text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
            assert result.returncode and 'AssertionError' in result.stderr,(script,options,result.stdout,result.stderr)
            assert before==sorted(path.relative_to(root) for path in root.rglob('*')) and sentinel.read_bytes()==b'unchanged\n'
            assert not output.exists();refusals+=1
    runner=here/'tests/run_pioneer_q38_token_original_flow_tests.py'
    required=['--cancel-boundary']+[value for value in common if value!='--f32-hidden']+[candidate]
    invalid=[required,[candidate],required+['--pushed-completion-faults','--first-layer-only'],
             required+['--pushed-completion-faults','--prepare-only'],required+['--pushed-completion-faults','--fast-root-memory'],
             required+['--pushed-completion-faults','--pushed-cohort-faults'],
             required+['--pushed-completion-faults','--pushed-admission-faults'],
             required+['--pushed-completion-faults','--pushed-dense-faults']]
    for options in invalid:
        before=sorted(path.relative_to(root) for path in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*[str(root/'unused')]*4,*options],
                              text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode and 'AssertionError' in result.stderr,(options,result.stdout,result.stderr)
        assert before==sorted(path.relative_to(root) for path in root.rglob('*')) and sentinel.read_bytes()==b'unchanged\n'
        refusals+=1
    first_layer=required+['--first-layer-only','--observed-trace',str(root/'forbidden-prefix')]
    invalid=[first_layer+[extra] for extra in ('--prepare-only','--bootstrap-only','--fast-root-memory',
             '--snapshot-first-layer','--eos-after-prompt','--admission-source-faults','--pushed-completion-faults',
             '--pushed-cohort-faults','--pushed-admission-faults','--pushed-dense-faults')]
    invalid += [[value for value in first_layer if value!='--pushed-completion'],
                required+['--observed-trace',str(root/'forbidden-full-model')],
                required+['--snapshot-first-layer','--observed-trace',str(root/'forbidden-full-model')]]
    for options in invalid:
        before=sorted(path.relative_to(root) for path in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*[str(root/'unused')]*4,*options],
                              text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode and 'AssertionError' in result.stderr,(options,result.stdout,result.stderr)
        assert before==sorted(path.relative_to(root) for path in root.rglob('*')) and sentinel.read_bytes()==b'unchanged\n'
        refusals+=1
    qualifier='--qualify-packet-seal-full-flow'
    full=required+[qualifier,'--snapshot-first-layer','--observed-trace',str(root/'forbidden-complete')]
    invalid=[full+[extra] for extra in ('--prepare-only','--bootstrap-only','--first-layer-only','--fast-root-memory',
             '--eos-after-prompt','--admission-source-faults','--pushed-completion-faults','--pushed-cohort-faults',
             '--pushed-admission-faults','--pushed-dense-faults')]
    invalid += [[value for value in full if value!=missing] for missing in (candidate,'--pushed-completion','--snapshot-first-layer')]
    invalid += [required+[qualifier,'--snapshot-first-layer'],full+[qualifier]]
    # This new BUILD selector grants NO connected/whole-model runtime mode.
    invalid += [required+[eight],first_layer+[eight],full+[eight]]
    for options in invalid:
        before=sorted(path.relative_to(root) for path in root.rglob('*'))
        result=subprocess.run([sys.executable,str(runner),*[str(root/'unused')]*4,*options],
                              text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        assert result.returncode and 'AssertionError' in result.stderr,(options,result.stdout,result.stderr)
        assert before==sorted(path.relative_to(root) for path in root.rglob('*')) and sentinel.read_bytes()==b'unchanged\n'
        refusals+=1
print(f'PASS packet-seal inspect-only BUILD CLI/{refusals} missing-parent+no-inspect+duplicate+unknown refusals/unchanged complete output inventory;NO compiler/device/native/Work authority')
