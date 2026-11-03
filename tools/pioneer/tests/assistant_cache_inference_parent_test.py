#!/usr/bin/env python3
"""Actual frozen inference-trial flow with fake commands, NEVER SSH/model/device."""
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_assistant_cache_inference_trial.sh').read_text()
flow = source.split('# INFERENCE_TRIAL_FLOW_BEGIN\n', 1)[1].split('# INFERENCE_TRIAL_FLOW_END', 1)[0]
folder = pathlib.Path(tempfile.mkdtemp(prefix='assistant-cache-inference-parent.'))
probe = folder / 'fake-probe'
probe.write_text('''#!/bin/sh
echo probe >> "$stage/events"
if [ "$mode" = fail ];then echo FAKE_FAILURE >&2;exit 70;fi
if [ "$mode" = warn ];then echo FAKE_WARNING >&2;fi
if [ "$mode" = malformed ];then echo WRONG_COMPLETION;else
 echo 'PASS_ACTUAL_ASSISTANT_CACHED_FOUR_TOKEN_WORK_KNOWN_CLEAN;NOT_FULL_NATIVE_NOT_20TOKENS_SEC_OR_COMPARATIVE_SPEEDUP_PROOF'
fi
''')
probe.chmod(0o700)
prefix = '''#!/bin/sh
set -eu
stage=$1;probe=$2;mode=$3;runtime=fake;export stage mode
idle(){ echo idle >> "$stage/events"; }
metadata(){ echo metadata >> "$stage/events"; }
health(){ echo health >> "$stage/events"; }
'''
program = folder / 'flow.sh'
program.write_text(prefix + flow)
for mode in ('pass', 'fail', 'warn', 'malformed'):
    stage = folder / mode
    stage.mkdir()
    run = subprocess.run(['sh', str(program), str(stage), str(probe), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    assert run.returncode == (0 if mode == 'pass' else 70 if mode == 'fail' else 1), (mode, run.stdout, run.stderr)
    assert events == (['probe', 'idle', 'metadata', 'health'] if mode == 'pass' else ['probe'])
    assert (stage / '.inference-four-once-861e-57fe').is_dir()
    if mode == 'pass':
        again = subprocess.run(['sh', str(program), str(stage), str(probe), mode], text=True, capture_output=True)
        assert again.returncode != 0 and (stage / 'events').read_text().splitlines() == events
for suffix in ('pid', 'state', 'stdout', 'stderr', 'exit'):
    stage = folder / ('existing-' + suffix)
    stage.mkdir()
    saved = stage / ('inference-four.' + suffix)
    saved.write_text('PRESERVE_EVIDENCE\n')
    run = subprocess.run(['sh', str(program), str(stage), str(probe), 'pass'], text=True, capture_output=True)
    assert run.returncode == 1 and not (stage / 'events').exists()
    assert saved.read_text() == 'PRESERVE_EVIDENCE\n'
    assert not (stage / '.inference-four-once-861e-57fe').exists()
stage = folder / 'dangling-output'
stage.mkdir()
(stage / 'inference-four.stdout').symlink_to('MISSING_TARGET')
run = subprocess.run(['sh', str(program), str(stage), str(probe), 'pass'], text=True, capture_output=True)
assert run.returncode == 1 and not (stage / 'events').exists()
assert (stage / 'inference-four.stdout').is_symlink()
print('PASS actual guarded cache-inference parent flow:11 fake-command cases;one shot,evidence preservation,known-clean checks only after valid success,failed/warning/malformed completion NO retry/metadata/cleanup;NO full preflight or physical proof')
print('CPU-only parent evidence:', folder)

# Exercise the ACTUAL closed-host admission gate separately with fake SHA/stat
# commands. Not a full binary/library/driver/native preflight simulation.
gate = source.split('# Closed host-only qualification,', 1)[1].split('# INFERENCE_TRIAL_FLOW_BEGIN', 1)[0]
gate = '# Closed host-only qualification,' + gate
gate_program = folder / 'closed-host-gate.sh'
gate_program.write_text('''#!/bin/sh
set -eu
stage=$1
exact(){ return 0; }
stat(){ printf '0:1\\n'; }
''' + gate + '\\necho ADMITTED_CLOSED_HOST_ONLY\\n'.replace('\\n', '\n'))
for mode in ('closed', 'missing', 'exit-nonzero', 'running', 'wrong-completion', 'stderr', 'symlink', 'directory', 'dangling'):
    stage = folder / ('gate-' + mode)
    stage.mkdir()
    (stage / 'host-progress.exit').write_text('0\n')
    (stage / 'host-progress.state').write_text('KNOWN_CLEAN_HOST_LOAD_UNLOAD_ALL851_VERIFIED_NO_GENERATE\n')
    (stage / 'host-progress.stdout').write_text('PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM\n')
    (stage / 'host-progress.stderr').write_text('')
    if mode == 'missing':
        (stage / 'host-progress.exit').unlink()
    elif mode == 'exit-nonzero':
        (stage / 'host-progress.exit').write_text('70\n')
    elif mode == 'running':
        (stage / 'host-progress.state').write_text('RUNNING_GUARDED_HOST_LOAD_UNLOAD_NO_GENERATE\n')
    elif mode == 'wrong-completion':
        (stage / 'host-progress.stdout').write_text('INCOMPLETE\n')
    elif mode == 'stderr':
        (stage / 'host-progress.stderr').write_text('warning\n')
    elif mode == 'symlink':
        (stage / 'host-progress.state').rename(stage / 'other-state')
        (stage / 'host-progress.state').symlink_to('other-state')
    elif mode == 'directory':
        (stage / 'host-progress.stdout').unlink()
        (stage / 'host-progress.stdout').mkdir()
    elif mode == 'dangling':
        (stage / 'host-progress.stderr').unlink()
        (stage / 'host-progress.stderr').symlink_to('MISSING')
    run = subprocess.run(['sh', str(gate_program), str(stage)], text=True, capture_output=True)
    assert (run.returncode == 0) == (mode == 'closed'), (mode, run.stdout, run.stderr)
    assert ('ADMITTED_CLOSED_HOST_ONLY' in run.stdout) == (mode == 'closed')
print('PASS actual closed-host admission gate:9 fake-stat/SHA cases;missing/nonzero/live/wrong/unsafe host evidence cannot admit inference;NOT full preflight or physical proof')
