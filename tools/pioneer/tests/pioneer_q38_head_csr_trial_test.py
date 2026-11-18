#!/usr/bin/env python3
"""Check the generated pair runner's exchange/cleanup control flow, without PCI I/O."""
from hashlib import sha256
import os
from pathlib import Path
import runpy
import subprocess
import sys
from tempfile import TemporaryDirectory


HERE = Path(__file__).resolve().parent
BUILDER = HERE.parent / 'build_qwen38_head_csr_trial.py'
SEED_TEST = HERE / 'pioneer_q38_tag206_phase_rate_once_test.py'
bind = runpy.run_path(str(BUILDER))['bind']
sys.path.insert(0,str(HERE.parent))
from build_qwen38_vector92_ffn_trial import bind as bind_ffn
from build_qwen38_vector_resident_trial import bind as bind_resident
from build_qwen38_layer_split_profile_trial import (bind as bind_layer_split, HEAD_MANIFEST,
                                                    WIRE_MANIFEST, OBSERVATION_MANIFEST, postflight, recipe_postflight)
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
RESIDENT_FAKES = FAKES.replace('--full-card-rate-32', '--full-card-resident-rate-32').replace(
    'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available1 output_tokens32;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY',
    'PASS_RESIDENT_TWO_REQUEST one_upload1 model_end1 worker_exit0 CPU_INFERENCE_CONTEXTS0;NOT_BROAD_QUALITY')
SPLIT_FAKES = FAKES.replace('/bin/cat "$stage/worker-stderr" >&2', '''/bin/cat "$stage/worker-stderr" >&2
 local valid=1 buckets=14
 if [ "$PIONEER_FAKE_CASE" = invalid-v4 ]; then valid=0; fi
 if [ "$PIONEER_FAKE_CASE" = incomplete-v4 ]; then buckets=13; fi
 printf 'FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile valid%s committed62 window_start31 last_position61 fixture\n' "$valid"
 for ((i=0;i<buckets;i++)); do
  printf 'FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile bucket%s fixture\n' "$i"
 done''')


def resident(variant, start):
    assert variant == 'vector'
    return bind_resident(start)


def layer_split(variant,start):
    assert variant in ('candidate','live','recipe','wire','head-raw','head-wire','observation')
    return bind_layer_split(start, variant=='live', recipe=variant=='recipe', wire=variant=='wire',
                            head=variant[5:] if variant.startswith('head-') else None,
                            observation=variant=='observation')

def main():
    for builder,name in ((bind,'serial'),(bind_ffn,'refetch'),(resident,'vector'),(layer_split,'candidate')):
        for bad in ('0', '01', ' 3', '2\n', '-1', '1000000', '3;exit'):
            try:
                builder(name,bad)
            except ValueError:
                pass
            else:
                raise AssertionError(bad)
    for bad in ('0', '01', ' 3', '2\n', '-1', '3;exit'):
        try:
            recipe_postflight('4',bad)
        except ValueError:
            pass
        else:
            raise AssertionError(bad)
    source=recipe_postflight('4','735')
    assert 'stage=/boot/home/pioneer-q38-layer-split-recipe-216\n' in source
    assert "printf '735\\n' | cmp -s launch.pid -" in source and '$1==735 ||' in source
    assert 'STABLE041_GEN6_RESTORED' in source and 'generation6 quiesced0 cold0 active_owner0' in source
    runner=layer_split('recipe','4')
    assert f'exact {sha256(runner.encode("ascii")).hexdigest()} run-once.sh' in source
    assert source.index('listing=$(ps -o Id -o Team)')<source.index('status=$("$admin" --status)')
    assert '--quiesce' not in source and '--wait' not in source
    subprocess.run(['bash','-n'],input=source,text=True,check=True)
    cases = ('preflight', 'stale', 'lock', 'link-lock', 'success', 'fail', 'bad-marker',
             'no-rate', 'stderr', 'owned', 'exchange-unknown')
    with TemporaryDirectory(prefix='pioneer-head-csr-rate-') as tmp:
        # 2 and 3 exercise overlapping old/new generation values, 15 the
        # hexadecimal wait boundary. The check also fixes per-variant tags.
        trials=((bind,'serial',2,'207'),(bind,'candidate',3,'208'),(bind,'serial',15,'207'),
                (bind_ffn,'refetch',6,'211'),(bind_ffn,'reuse',15,'212'),(resident,'vector',8,'210'),
                (layer_split,'candidate',10,'213'),(layer_split,'live',12,'214'),(layer_split,'recipe',14,'216'),
                (layer_split,'wire',16,'217'),(layer_split,'head-raw',18,'218'),(layer_split,'head-wire',20,'219'),
                (layer_split,'observation',4,'224'))
        tested=0
        for index, (builder, name, start, suffix) in enumerate(trials):
            source = builder(name, str(start))
            tag = '2026111700010'+suffix
            assert f'candidateTag={tag}' in source
            assert 'pairLock=/boot/home/.pioneer-q38-head-csr-exclusive' in source
            assert f'metadata "$stableTag" {start} &&' in source
            assert f'--wait "$candidateTag" {start:016x}' in source
            assert f'--wait "$stableTag" {start+1:016x}' in source
            assert 'READINESS_Q4' not in source
            if builder is resident:
                assert 'stage=/boot/home/pioneer-q38-resident-vector-210\n' in source
                assert 'client=$stage/pioneer-q38-full-offload-resident-rate\n' in source
                assert '--full-card-resident-rate-32' in source and 'PASS_FULL_OFFLOAD_RATE_JOB' not in source
                assert 'one_upload1 model_end1 worker_exit0' in source
            manifest=None
            if builder is layer_split:
                stage_name='layer-split-'+name
                if name=='wire':stage_name='native-q8-wire';manifest=WIRE_MANIFEST
                if name=='observation':stage_name='native-q8-observation';manifest=OBSERVATION_MANIFEST
                if name.startswith('head-'):
                    stage_name='profile-'+name;manifest=HEAD_MANIFEST[name[5:]]
                written=f'stage=/boot/home/pioneer-q38-{stage_name}-{suffix}\n'
                assert written in source
                assert 'missing_closed_v4_timing' in source
                if manifest:
                    check=f'exact "$stage/manifest.json" {manifest} || fail manifest'
                    assert source.count(check)==1 and source.index(check)<source.index('mark(){')
                    closed=postflight(str(start),'731',name)
                    assert written in closed and f'exact {manifest} manifest.json' in closed
                    assert f'exact {sha256(source.encode("ascii")).hexdigest()} run-once.sh' in closed
                    assert f'STABLE041_GEN{start+2}_RESTORED' in closed
                    subprocess.run(['bash','-n'],input=closed,text=True,check=True)
            subprocess.run(['bash', '-n'], input=source, text=True, check=True)
            head, body = source.split('\n[ "$#" -eq 1 ]', 1)
            body = '\n[ "$#" -eq 1 ]' + body
            for case in cases+(('invalid-v4','incomplete-v4') if builder is layer_split else ())+(
                    ('wrong-manifest',) if manifest else ()):
                tested+=1
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
                fake = RESIDENT_FAKES if builder is resident else SPLIT_FAKES if builder is layer_split else FAKES
                if manifest:
                    # Model the exact manifest check extracted from the
                    # bound preflight; wrong contents must precede exchange.
                    fake=fake.replace('inputs(){',f'inputs(){{\n {check}')
                    fake=fake.replace(' *) return 1;;\n esac; }',
                                      f' "{manifest}") test "$(< "$1")" = manifest;;\n *) return 1;;\n esac; }}')
                    (stage/'manifest.json').write_text('wrong\n' if case=='wrong-manifest' else 'manifest\n')
                cmd = ['bash', '-c', head + fake + body, 'mock-head-csr-parent', mode]
                env = {**os.environ, 'PIONEER_FAKE_STAGE': str(stage), 'PIONEER_FAKE_CASE': case,
                       'PIONEER_FAKE_GEN': str(start)}
                result = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
                assert result.returncode == (0 if case in ('preflight', 'success') else 1), result
                if case in ('invalid-v4','incomplete-v4'):assert 'missing_closed_v4_timing' in result.stderr
                mutations = stage / 'mutations'
                once = stage / '.trial-once'
                if case in ('preflight', 'stale', 'lock', 'link-lock','wrong-manifest'):
                    if case=='wrong-manifest':assert ' manifest' in result.stderr
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
    print(f'PASS: {tested} fake raw/FFN/resident/v4 cases, stale/exclusive/spent, manifest, clean restore or lock retained; no card contact')


if __name__ == '__main__':
    main()
