#!/usr/bin/env python3
"""Check the generated pair runner's exchange/cleanup control flow, without PCI I/O."""
import os
from pathlib import Path
import runpy
import subprocess
from tempfile import TemporaryDirectory


HERE = Path(__file__).resolve().parent
BUILDER = HERE.parent / 'build_qwen38_head_csr_trial.py'
SEED_TEST = HERE / 'pioneer_q38_tag206_phase_rate_once_test.py'
bind = runpy.run_path(str(BUILDER))['bind']
FAKES = runpy.run_path(str(SEED_TEST))['FAKES']
FAKES = FAKES.replace('installed=$stage/installed', 'pairLock=$stage/shared-lock\ninstalled=$stage/installed')
FAKES = FAKES.replace('metadata "$stableTag" 1', 'metadata "$stableTag" "$PIONEER_FAKE_GEN"')
FAKES = FAKES.replace('paths && test ! -e "$stage/.trial-once"',
                      'paths && test ! -e "$pairLock" && test ! -L "$pairLock" && test ! -e "$stage/.trial-once"')
FAKES = FAKES.replace('metadata "$candidateTag" 2', 'metadata "$candidateTag" "$((PIONEER_FAKE_GEN+1))"')
FAKES = FAKES.replace("printf '%s 2 quiesced0 cold1 active_owner1\\n' \"$candidateTag\"",
                      "printf '%s %s quiesced0 cold1 active_owner1\\n' \"$candidateTag\" \"$((PIONEER_FAKE_GEN+1))\"")
FAKES = FAKES.replace('bad-marker) return 0;;', '''bad-marker) return 0;;
 no-rate)
  printf '%s\\n' 'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available0 output_tokens32;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY'
  return 0;;''')
FAKES = FAKES.replace('--quiesce)\n  test', '--quiesce)\n  if [ "$PIONEER_FAKE_CASE" = exchange-unknown ]; then return 1; fi\n  test')


def main():
    for bad in ('0', '01', ' 3', '2\n', '-1', '1000000', '3;exit'):
        try:
            bind('serial', bad)
        except ValueError:
            pass
        else:
            raise AssertionError(bad)
    cases = ('preflight', 'stale', 'lock', 'link-lock', 'success', 'fail', 'bad-marker',
             'no-rate', 'stderr', 'owned', 'exchange-unknown')
    with TemporaryDirectory(prefix='pioneer-head-csr-rate-') as tmp:
        # 2 and 3 exercise overlapping old/new generation values, 15 the
        # hexadecimal wait boundary. The check also fixes per-variant tags.
        for index, (name, start) in enumerate((('serial', 2), ('candidate', 3), ('serial', 15))):
            source = bind(name, str(start))
            tag = f'202611170001020{7 if name == "serial" else 8}'
            assert f'candidateTag={tag}' in source
            assert f'metadata "$stableTag" {start} &&' in source
            assert f'--wait "$candidateTag" {start:016x}' in source
            assert f'--wait "$stableTag" {start+1:016x}' in source
            assert 'READINESS_Q4' not in source
            subprocess.run(['bash', '-n'], input=source, text=True, check=True)
            head, body = source.split('\n[ "$#" -eq 1 ]', 1)
            body = '\n[ "$#" -eq 1 ]' + body
            for case in cases:
                stage = Path(tmp) / f'{index}-{case}'
                stage.mkdir()
                for file, content in {'installed': 'stable', 'test-stable': 'stable',
                                      'test-candidate': 'candidate'}.items():
                    (stage / file).write_text(content + '\n')
                actual = start+1 if case == 'stale' else start
                (stage / 'manager').write_text(f'2026102100010012 {actual} quiesced0 cold0 active_owner0\n')
                (stage / 'worker-stderr').write_bytes(b'unexpected\n' if case == 'stderr' else b'')
                lock = stage / 'shared-lock'
                if case == 'lock':
                    lock.mkdir()
                elif case == 'link-lock':
                    lock.symlink_to(stage / 'nonexistent')
                mode = '--preflight' if case == 'preflight' else '--run'
                cmd = ['bash', '-c', head + FAKES + body, 'mock-head-csr-parent', mode]
                env = {**os.environ, 'PIONEER_FAKE_STAGE': str(stage), 'PIONEER_FAKE_CASE': case,
                       'PIONEER_FAKE_GEN': str(start)}
                result = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
                assert result.returncode == (0 if case in ('preflight', 'success') else 1), result
                mutations = stage / 'mutations'
                once = stage / '.trial-once'
                if case in ('preflight', 'stale', 'lock', 'link-lock'):
                    assert not mutations.exists() and not once.exists(), case
                    continue
                assert once.is_dir()
                if case == 'exchange-unknown':
                    assert not mutations.exists() and lock.is_dir()
                    assert (stage / 'installed').read_text() == 'stable\n'
                else:
                    events = mutations.read_text().splitlines()
                    assert events[:2] == ['--quiesce 2026102100010012', '--wait ' + tag], (case, events)
                    if case == 'owned':
                        assert len(events) == 2 and lock.is_dir()
                        assert (stage / 'installed').read_text() == 'candidate\n'
                    else:
                        assert events[2:] == ['--quiesce ' + tag, '--wait 2026102100010012'], (case, events)
                        assert not lock.exists() and (stage / 'installed').read_text() == 'stable\n'
                        assert (stage / 'state').read_text() == f'STABLE041_GEN{start+2}_RESTORED\n'
                previous = mutations.read_bytes() if mutations.exists() else b''
                spent = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
                assert spent.returncode == 1
                assert (mutations.read_bytes() if mutations.exists() else b'') == previous
    print('PASS: both variants, 33 fake cases, stale/exclusive/spent, clean restore or lock retained; no card contact')


if __name__ == '__main__':
    main()
