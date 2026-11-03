#!/usr/bin/env python3
"""Actual frozen host-trial flow with fake commands, NEVER SSH/model/device."""
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_assistant_cache_host_progress_trial.sh').read_text()
flow = source.split('# HOST_PROGRESS_TRIAL_FLOW_BEGIN\n', 1)[1].split('# HOST_PROGRESS_TRIAL_FLOW_END', 1)[0]
folder = pathlib.Path(tempfile.mkdtemp(prefix='assistant-cache-host-progress-parent.'))
probe = folder / 'fake-probe'
probe.write_text('''#!/bin/sh
echo probe >> "$stage/events"
if [ "$mode" = fail ];then echo FAKE_FAILURE >&2;exit 70;fi
if [ "$mode" = warn ];then echo FAKE_WARNING >&2;fi
if [ "$mode" = malformed ];then echo WRONG_COMPLETION;else
 echo 'PASS_ACTUAL_ASSISTANT_CACHE_LOAD_UNLOAD verified_tensors851;NO_GENERATE_NO_WEIGHT_UPLOAD_NO_INFERENCE_SPEED_CLAIM'
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
    assert (stage / '.host-progress-once-861e-5578').is_dir()
    if mode == 'pass':
        again = subprocess.run(['sh', str(program), str(stage), str(probe), mode], text=True, capture_output=True)
        assert again.returncode != 0 and (stage / 'events').read_text().splitlines() == events
for suffix in ('pid', 'state', 'stdout', 'stderr', 'exit'):
    stage = folder / ('existing-' + suffix)
    stage.mkdir()
    saved = stage / ('host-progress.' + suffix)
    saved.write_text('PRESERVE_EVIDENCE\n')
    run = subprocess.run(['sh', str(program), str(stage), str(probe), 'pass'], text=True, capture_output=True)
    assert run.returncode == 1 and not (stage / 'events').exists()
    assert saved.read_text() == 'PRESERVE_EVIDENCE\n'
    assert not (stage / '.host-progress-once-861e-5578').exists()
stage = folder / 'dangling-output'
stage.mkdir()
(stage / 'host-progress.stdout').symlink_to('MISSING_TARGET')
run = subprocess.run(['sh', str(program), str(stage), str(probe), 'pass'], text=True, capture_output=True)
assert run.returncode == 1 and not (stage / 'events').exists()
assert (stage / 'host-progress.stdout').is_symlink()
print('PASS actual guarded cache-host-progress parent flow:11 fake-command cases;one shot,evidence preservation,known-clean checks only after valid success,failed/warning/malformed completion NO retry/metadata/cleanup;NO full preflight or physical proof')
print('CPU-only parent evidence:', folder)
