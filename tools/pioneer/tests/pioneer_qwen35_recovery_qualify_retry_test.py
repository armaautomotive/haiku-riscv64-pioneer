#!/usr/bin/env python3
"""Exercise the second qualification's existing-guard and failure paths."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

import pioneer_qwen35_recovery_qualify_script_test as common


def exercise(suffix, script, candidate_hash):
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        home, old_stage, installed, guard, _, env = common.fixture(root)
        stage = old_stage.with_name(old_stage.name + suffix)
        old_stage.rename(stage)
        (stage/'guard.previous').write_text('old guard')
        (stage/'guard.previous').chmod(0o755)
        guard.write_text('new guard')
        driver_state = Path(env['MOCK_STATE'])
        driver_state.write_text(json.dumps(dict(driver=55, tag='2026102100010012', generation=2,
                                                quiesced=False, cold=False, active=False)))
        source = (common.ROOT/'tools/pioneer'/script).read_text()
        source = source.replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        source = source.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for old, path in ((common.OLD_DRIVER, installed), (common.OLD_GUARD, stage/'guard.previous'),
                          (common.NEW_GUARD, guard),
                          (common.ADMIN, home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'),
                          (candidate_hash, stage/'candidate'),
                          ('34af989cb81827835f8e1fe762c8018ec5dbbb686443df992a15dea5fee13abc', stage/'client')):
            source = source.replace(old, hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper = root/'retry.sh'
        common.put(wrapper, source)
        def run(mode, extra=None):
            return subprocess.run(['bash', str(wrapper), mode, '2'], env=dict(env, **(extra or {})),
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert run('--prepare-existing-guard').returncode == 0
        result = run('--run', {'MOCK_FAIL': '1'})
        assert result.returncode != 0 and installed.read_text() == 'candidate'
        assert (stage/'state').read_text().strip() == 'QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
        assert (home/'.pioneer-q35-dataflow-exclusive').is_dir()
        assert run('--run').returncode != 0
        driver_state.write_text(json.dumps(dict(driver=60, tag='2026112300030001', generation=1,
                                                quiesced=False, cold=False, active=False)))
        restore = common.RESTORE.read_text().replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        restore = restore.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for old, path in ((common.OLD_DRIVER, stage/'restore.incoming'), (common.NEW_GUARD, guard),
                          (common.ADMIN, home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'),
                          (common.CANDIDATE, stage/'candidate'),
                          ('cabebd7d42b60106c6d97216353400bbfd76a58b0f982b24389715dbc95f3e70', stage/'candidate'),
                          ('1326f14e7d2f5997b2f2048e2308fc51747b46b949e6cd7417a78a474b61b292', stage/'candidate'),
                          (candidate_hash, stage/'candidate')):
            restore = restore.replace(old, hashlib.sha256(path.read_bytes()).hexdigest())
        restore_path = root/'restore.sh'
        common.put(restore_path, restore)
        result = subprocess.run(['bash', str(restore_path), str(stage)], env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert installed.read_text() == 'stable' and not (home/'.pioneer-q35-dataflow-exclusive').exists()


def main():
    exercise('-b', 'pioneer_qwen35_recovery_qualify_retry.sh', common.CANDIDATE)
    exercise('-c', 'pioneer_qwen35_recovery_qualify_firmware.sh',
             'cabebd7d42b60106c6d97216353400bbfd76a58b0f982b24389715dbc95f3e70')
    exercise('-d', 'pioneer_qwen35_recovery_qualify_audit.sh',
             '1326f14e7d2f5997b2f2048e2308fc51747b46b949e6cd7417a78a474b61b292')
    exercise('-e', 'pioneer_qwen35_recovery_qualify_dma.sh',
             'bdbcc8dc0d9823c2e249dba7eb508289be2332d70b78fe5174c3966f8841c265')
    print('PASS existing-guard qualifications and retained failure; SIMULATED HOST ONLY')


if __name__ == '__main__':
    main()
