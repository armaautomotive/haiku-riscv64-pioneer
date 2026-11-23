#!/usr/bin/env python3
"""Exercise the second qualification's existing-guard and failure paths."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

import pioneer_qwen35_recovery_qualify_script_test as common


def exercise(suffix, script, candidate_hash, client_hash='34af989cb81827835f8e1fe762c8018ec5dbbb686443df992a15dea5fee13abc', reject=False, succeed=False):
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
                          (client_hash, stage/'client')):
            source = source.replace(old, hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper = root/'retry.sh'
        common.put(wrapper, source)
        def run(mode, extra=None):
            return subprocess.run(['bash', str(wrapper), mode, '2'], env=dict(env, MOCK_AER_FORMAT='1' if suffix in ('-g','-h','-i','-j','-k') else '0', **(extra or {})),
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert run('--prepare-existing-guard').returncode == 0
        if succeed:
            result = run('--run')
            assert result.returncode == 0, (result.stdout, result.stderr)
            assert installed.read_text() == 'stable' and (stage/'state').read_text().strip() == 'RECOVERY_QUALIFIED_STABLE_RESTORED'
            assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
            return
        if reject:
            result = run('--run', {'MOCK_REJECT': '1'})
            assert result.returncode != 0 and installed.read_text() == 'stable'
            assert (stage/'state').read_text().strip() == 'PRE_RESET_PCI_REJECTED_STABLE_RESTORED'
            assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
            return
        result = run('--run', {'MOCK_FAIL': '1'})
        assert result.returncode != 0 and installed.read_text() == 'candidate'
        assert (stage/'state').read_text().strip() == 'QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
        assert (home/'.pioneer-q35-dataflow-exclusive').is_dir()
        assert run('--run').returncode != 0
        driver_state.write_text(json.dumps(dict(driver=60, tag='2026112300030002' if suffix=='-k' else '2026112300030001', generation=1,
                                                quiesced=False, cold=False, active=False)))
        restore = common.RESTORE.read_text().replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        restore = restore.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for old, path in ((common.OLD_DRIVER, stage/'restore.incoming'), (common.NEW_GUARD, guard),
                          (common.ADMIN, home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'),
                          (common.CANDIDATE, stage/'candidate'),
                          ('cabebd7d42b60106c6d97216353400bbfd76a58b0f982b24389715dbc95f3e70', stage/'candidate'),
                          ('1326f14e7d2f5997b2f2048e2308fc51747b46b949e6cd7417a78a474b61b292', stage/'candidate'),
                          ('bdbcc8dc0d9823c2e249dba7eb508289be2332d70b78fe5174c3966f8841c265', stage/'candidate'),
                          ('138e21ff9cd5ecc6d3c734dac835189815164532af276df8984b6550d312e60c', stage/'candidate'),
                          ('698b972450841061a27d33d4d7853a708092ede1539315788fff5afa9d4700f9', stage/'candidate'),
                          ('e685a5aa888a6b0ddaeb61245c0455a4dc6181b30cc5d005e87c15d05ab698bb', stage/'candidate'),
                          ('034d147f06c2192371e00c447e5e340844ccd92683a054c370eb3a716aa03c27', stage/'candidate'),
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
    exercise('-f', 'pioneer_qwen35_recovery_qualify_pci_baseline.sh',
             '138e21ff9cd5ecc6d3c734dac835189815164532af276df8984b6550d312e60c',
             '2ac40951a28bd9da2bdbb9aa1f6a713a05b52fbc0aafe78fbcc11bb32b588bbf')
    exercise('-f', 'pioneer_qwen35_recovery_qualify_pci_baseline.sh',
             '138e21ff9cd5ecc6d3c734dac835189815164532af276df8984b6550d312e60c',
             '2ac40951a28bd9da2bdbb9aa1f6a713a05b52fbc0aafe78fbcc11bb32b588bbf', reject=True)
    exercise('-g', 'pioneer_qwen35_recovery_qualify_root_aer.sh',
             '698b972450841061a27d33d4d7853a708092ede1539315788fff5afa9d4700f9',
             '1ffd7dd54584ac13a0f768b5446405973fbf18b5d6fc829c9ca2e58f16b43c1c')
    exercise('-g', 'pioneer_qwen35_recovery_qualify_root_aer.sh',
             '698b972450841061a27d33d4d7853a708092ede1539315788fff5afa9d4700f9',
             '1ffd7dd54584ac13a0f768b5446405973fbf18b5d6fc829c9ca2e58f16b43c1c', reject=True)
    exercise('-h', 'pioneer_qwen35_recovery_qualify_aer_baseline.sh',
             'e685a5aa888a6b0ddaeb61245c0455a4dc6181b30cc5d005e87c15d05ab698bb',
             '1ffd7dd54584ac13a0f768b5446405973fbf18b5d6fc829c9ca2e58f16b43c1c')
    exercise('-h', 'pioneer_qwen35_recovery_qualify_aer_baseline.sh',
             'e685a5aa888a6b0ddaeb61245c0455a4dc6181b30cc5d005e87c15d05ab698bb',
             '1ffd7dd54584ac13a0f768b5446405973fbf18b5d6fc829c9ca2e58f16b43c1c', reject=True)
    exercise('-i', 'pioneer_qwen35_recovery_qualify_pcie_trace.sh',
             '034d147f06c2192371e00c447e5e340844ccd92683a054c370eb3a716aa03c27',
             '1b3a01c993e200ddab2fe88cf7ad3f40f9315cdecdc0a7c2de8a28dbadbbfc5d')
    exercise('-i', 'pioneer_qwen35_recovery_qualify_pcie_trace.sh',
             '034d147f06c2192371e00c447e5e340844ccd92683a054c370eb3a716aa03c27',
             '1b3a01c993e200ddab2fe88cf7ad3f40f9315cdecdc0a7c2de8a28dbadbbfc5d', reject=True)
    exercise('-j', 'pioneer_qwen35_recovery_qualify_reset_phase.sh',
             'f3b8951e9d6c47a94d298a004a9fbb8833870ee42013fcfd4632ce0817123aa8',
             '9ff0ce212b9bb593202d92a2065d9bc77ccac25125b2c03f025592bc6a608e21')
    exercise('-j', 'pioneer_qwen35_recovery_qualify_reset_phase.sh',
             'f3b8951e9d6c47a94d298a004a9fbb8833870ee42013fcfd4632ce0817123aa8',
             '9ff0ce212b9bb593202d92a2065d9bc77ccac25125b2c03f025592bc6a608e21', reject=True)
    for options in ({}, {'reject': True}, {'succeed': True}):
        exercise('-k', 'pioneer_qwen35_recovery_qualify_firmware_reset.sh',
                 '638b3a74449e0574ccfe11e0ba078632f1c7f19c3e61d0ef64326ab04a2b5c15',
                 '50ef9ee8086c94a4149cabd889fefcd824e9955897df25bbe8dfdde5cae7986d', **options)
    print('PASS existing-guard qualifications and retained failure; SIMULATED HOST ONLY')


if __name__ == '__main__':
    main()
