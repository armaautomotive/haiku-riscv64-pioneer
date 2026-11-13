#!/usr/bin/env python3
"""Exercise the actual warm trial's post-admission flow with a fake probe."""
import pathlib
import subprocess
import tempfile

if not __debug__:
    raise SystemExit('Assertions required')
source = (pathlib.Path(__file__).resolve().parent.parent /
          'pioneer_batched_cache_warm_trial_20261112.sh').read_text()
flow = source.split('for suffix in pid state stdout stderr exit; do', 1)[1]
flow = 'for suffix in pid state stdout stderr exit; do' + flow
root = pathlib.Path(tempfile.mkdtemp(prefix='assistant-warm-parent.'))
program = root / 'flow.sh'
program.write_text('''#!/bin/sh
set -eu
stage=$1; mode=$2; runtime=fake; export stage mode
idle(){ echo idle >> "$stage/events"; [ "$mode" != idle-fail ]; }
metadata(){ echo metadata >> "$stage/events"; [ "$mode" != metadata-fail ]; }
health(){ echo health >> "$stage/events"; [ "$mode" != health-fail ]; }
''' + flow)
probe = '''#!/bin/sh
echo probe >> "$stage/events"
if [ "$mode" = fail ]; then echo failed >&2; exit 70; fi
if [ "$mode" = warn ]; then echo warning >&2; fi
if [ "$mode" = malformed ]; then echo wrong; else
 echo 'PASS_ACTUAL_ASSISTANT_TWO_FOUR_TOKEN_WORK_REQUESTS_KNOWN_CLEAN;NO_REUPLOAD_PROGRESS_REPORTED;NOT_FULL_NATIVE_OR_SPEEDUP_PROOF'
fi
'''
for mode in ('pass', 'fail', 'warn', 'malformed', 'idle-fail', 'metadata-fail', 'health-fail'):
    stage = root / mode
    stage.mkdir()
    (stage / 'warm-probe').write_text(probe)
    (stage / 'warm-probe').chmod(0o700)
    result = subprocess.run(['sh', str(program), str(stage), mode], capture_output=True, text=True)
    assert result.returncode == (0 if mode == 'pass' else 70 if mode == 'fail' else 1), (mode, result.stderr)
    events = (stage / 'events').read_text().splitlines()
    expected = {'pass': ['probe', 'idle', 'metadata', 'health'],
                'idle-fail': ['probe', 'idle'],
                'metadata-fail': ['probe', 'idle', 'metadata'],
                'health-fail': ['probe', 'idle', 'metadata', 'health']}
    assert events == expected.get(mode, ['probe'])
    assert (stage / '.warm-once').is_dir()
    if mode.endswith('-fail'):
        assert (stage / 'warm.state').read_text() == 'WARM_POSTFLIGHT_UNPROVEN_NO_RETRY\n'
    if mode == 'pass':
        again = subprocess.run(['sh', str(program), str(stage), mode], capture_output=True, text=True)
        assert again.returncode != 0 and (stage / 'events').read_text().splitlines() == events
for suffix in ('pid', 'state', 'stdout', 'stderr', 'exit'):
    stage = root / ('existing-' + suffix)
    stage.mkdir()
    evidence = stage / ('warm.' + suffix)
    evidence.write_text('preserved\n')
    result = subprocess.run(['sh', str(program), str(stage), 'pass'], capture_output=True, text=True)
    assert result.returncode == 1 and evidence.read_text() == 'preserved\n'
    assert not (stage / '.warm-once').exists()
stage = root / 'dangling'
stage.mkdir()
(stage / 'warm.stdout').symlink_to('missing')
result = subprocess.run(['sh', str(program), str(stage), 'pass'], capture_output=True, text=True)
assert result.returncode == 1 and (stage / 'warm.stdout').is_symlink()
print('PASS actual warm trial post-admission flow: 13 fake cases; one-shot evidence preservation and fail-closed checks; no model or hardware proof')
