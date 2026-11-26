#!/usr/bin/env python3
"""Exercise the ASIC qualification and restoration on a disposable mock host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

import pioneer_qwen35_recovery_qualify_script_test as base

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'tools/pioneer/pioneer_qwen35_recovery_qualify_asic_root.sh'
CANDIDATE = 'b660bb61458cf566c574c1bc4c3c2cd96606b4f16bd8e373e05cb16f14db5fcd'
CLIENT = '570ea928802482bba69e8d15f55cfbe5e91a3ac68f544a86976f0084d5755454'
TAG = '2026112500030024'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixture(root):
    home, old_stage, installed, guard, _, env = base.fixture(root)
    stage = home / 'pioneer-q35-asic-root-20261125-a'
    old_stage.rename(stage)
    (stage / 'guard.previous').write_text('old guard')
    guard.write_text('new guard')
    admin = home / 'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
    admin.write_text(admin.read_text().replace("'2026112300030002'", "'2026112300030002','" + TAG + "'"))
    substitutions = ((base.OLD_DRIVER, digest(installed)), (base.OLD_GUARD, digest(stage/'guard.previous')),
                     (base.NEW_GUARD, digest(guard)), (base.ADMIN, digest(admin)),
                     (CANDIDATE, digest(stage/'candidate')), (CLIENT, digest(stage/'client')))
    def translate(text):
        text = text.replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        text = text.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for original, replacement in substitutions:
            text = text.replace(original, replacement)
        return text
    wrapper = root/'asic-qualify.sh'
    restore = root/'restore.sh'
    base.put(wrapper, translate(SCRIPT.read_text()))
    base.put(restore, translate(base.RESTORE.read_text()))
    return home, stage, installed, wrapper, restore, env


def call(script, env, *args):
    return subprocess.run(['bash', str(script), *args], env=env, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def check(result, success):
    assert (result.returncode == 0) == success, (result.returncode, result.stdout, result.stderr)


def main():
    for outcome in ('success', 'reject', 'failure'):
        with tempfile.TemporaryDirectory() as temp:
            home, stage, installed, wrapper, restore, env = fixture(Path(temp))
            locks = [home/'.pioneer-q35-dataflow-exclusive', home/'.pioneer-q38-head-csr-exclusive']
            check(call(wrapper, env, '--prepare-existing-guard', '1'), True)
            assert all((lock/'owner').read_text().strip() == str(stage) for lock in locks)
            run_env = dict(env)
            if outcome == 'reject':
                run_env.update(MOCK_REJECT='1', MOCK_AER_FORMAT='1')
            if outcome == 'failure':
                run_env.update(MOCK_FAIL='1')
            check(call(wrapper, run_env, '--run', '1'), outcome == 'success')
            assert (stage/'.qualification-once').is_dir()
            if outcome != 'failure':
                expected = 'RECOVERY_QUALIFIED_STABLE_RESTORED' if outcome == 'success' else 'PRE_RESET_PCI_REJECTED_STABLE_RESTORED'
                assert (stage/'state').read_text().strip() == expected
                assert installed.read_text() == 'stable' and all(not lock.exists() for lock in locks)
                check(call(wrapper, env, '--run', '1'), False)
            else:
                assert installed.read_text() == 'candidate' and all(lock.is_dir() for lock in locks)
                assert (stage/'state').read_text().strip() == 'QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
                # Simulate only the metadata of a fresh boot, not physical recovery.
                Path(env['MOCK_STATE']).write_text(json.dumps(dict(driver=60, tag=TAG, generation=1,
                                                                   quiesced=False, cold=False, active=False)))
                # Missing a fleet lock must block restoration and retain the candidate.
                (locks[1]/'owner').write_text('different owner\n')
                check(call(restore, env, str(stage)), False)
                assert installed.read_text() == 'candidate'
                (locks[1]/'owner').write_text(str(stage)+'\n')
                check(call(restore, env, str(stage)), True)
                assert installed.read_text() == 'stable' and all(not lock.exists() for lock in locks)
    for state in ('QUALIFICATION_RUNNING', 'RESTORE_REQUIRED'):
        with tempfile.TemporaryDirectory() as temp:
            home, stage, installed, wrapper, restore, env = fixture(Path(temp))
            check(call(wrapper, env, '--prepare-existing-guard', '1'), True)
            check(call(wrapper, dict(env, MOCK_FAIL='1'), '--run', '1'), False)
            (stage/'state').write_text(state+'\n')
            Path(env['MOCK_STATE']).write_text(json.dumps(dict(driver=60, tag=TAG, generation=1,
                                                               quiesced=False, cold=False, active=False)))
            check(call(restore, env, str(stage)), True)
            assert installed.read_text() == 'stable'
    with tempfile.TemporaryDirectory() as temp:
        home, stage, installed, wrapper, _, env = fixture(Path(temp))
        (home/'.pioneer-q38-head-csr-exclusive').mkdir()
        check(call(wrapper, env, '--prepare-existing-guard', '1'), False)
        assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
        assert installed.read_text() == 'stable'
    with tempfile.TemporaryDirectory() as temp:
        home, stage, installed, wrapper, _, env = fixture(Path(temp))
        base.put(Path(temp)/'bin/ps', '#!/bin/sh\nprintf "   Id Team\\n    1 kernel_team\\n    2 pioneer_qwen27\\n"\n')
        check(call(wrapper, env, '--prepare-existing-guard', '1'), False)
        assert installed.read_text() == 'stable'
    print('PASS ASIC qualification success, rejection, retained failure, restoration, locks and one-shot gate; MOCK HOST ONLY')


if __name__ == '__main__':
    main()
