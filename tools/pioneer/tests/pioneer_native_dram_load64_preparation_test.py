#!/usr/bin/env python3
"""Immutable C64-control PREPARATION,never physical byte/math/model proof."""
import hashlib
import pathlib
import re

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-load64-preparation-20261104'
records = {}
for line in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
assert len(records) == 52 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for name in records:
    if name.endswith('.stderr'):
        if name.startswith('failed-probe'):
            assert 'AssertionError' in text(name)
        else:
            assert not text(name), name
build = text('build83-frozen.log')
for pin in ('fa70be17ca4b3cf3889e5176f6a651b2f5afcdefd0511f98aba4745d6c2b04b7',
            '7ee39a3615bc93e3b81cd21149fdd020f2e5f3e7a7f1b3db8cca24bffd94a85e',
            '10310865e07c34b476632e8e776f76fae603a7a4adea2ec87f226f1f05798ac0',
            '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'):
    assert pin in build
assert '18 forbidden roles/2 actual placement compile refusals/legacy NULL/default unchanged' in build
assert len(text('driver-role-refusals83.txt').splitlines()) == 18
assert text('placement-compile-refusals83.txt') == ('PASS cached-opposite-policy-exact-driver-binding\n'
    'PASS opposite-placement-image-full-driver-instantiation\n')
assert text('placement-compile-refusals82.txt') == text('placement-compile-refusals83.txt')
assert len(text('driver-role-refusals82.txt').splitlines()) == 16
assert '8 route-shape valid/21 frame/NIU/FIFO/counter/header/job faults' in text('image83-frozen.log')
assert 'marker3436444e' in text('image83-frozen.log') and 'steps61797' in text('image83-frozen.log')
lease = text('lease83-frozen.log')
assert '258 mismatch/failure/admission cases' in lease
assert lease.count('46080 payload+5120 header words exact') == 3
for sample in range(3):
    assert lease.count('PASS OFFBOARD actual LOAD ONLY sample%d:' % sample) == 1
for name, needle in (
        ('native83-frozen.log', 'valid308 refused669'),
        ('client83-final.log', 'client:231 ASan/UBSan cases'),
        ('parent83-final.log', '29 flow/output+7 idle+11 failed-prior/recovery/model+20 exchange-step+10 one-shot cases'),
        ('loader64-final.log', '27 valid/36 refused'),
        ('parser83-final.log', '3 valid/64 refused'),
        ('native82-final.log', 'valid308 refused665'),
        ('math81-native-final.log', 'valid307 refused643'),
        ('client82-final.log', 'client:228 ASan/UBSan cases'),
        ('parent82-final.log', '27 flow/output+7 idle+11 failed-prior/recovery/model+20 exchange-step+10 one-shot cases'),
        ('lease82-final.log', '162 mismatch/failure/admission cases'),
        ('parser82-final.log', '3 valid/62 refused')):
    assert needle in text(name)
for pin in ('3df4fdb1d034c39df5c24bc06735012109d3881431c5a9ddaedd16ecc535d3d6',
            'f7ae6e1be5fbcf6618ddd4326715c49166169a0073f868a86ec353519bc4f719',
            '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'):
    assert pin in text('control82-binding-r2.log')
for pin in ('0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27',
            '0091244ae265e26075e80fa98f5d6b8b5c6d37304938e3d140d9bab6584e307e'):
    assert pin in text('math81-build-final.log')
for pin in ('cd924896986dabcead8493c9dedca1d7b25fde323fbd972caf67ad93139a9309',
            '13494a0a5fc262b02dc7b7a6353da5e773a5ec01d732efd29d6525ba58a6e5f2',
            'ee740295aeffdb555c5fa0706d092e6d1a467b38b6351bc067bbfe7edbb57a45'):
    assert pin in text('math-images-final.log')
preflight = text('preflight-readonly.log')
assert preflight.startswith('Haiku\nP100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n')
assert '64 (Unknown)' in preflight and '/ 137034936320)' in preflight
assert 'UNPROVEN_LOAD_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE' in preflight
assert 'NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED' in preflight
assert preflight.endswith('READONLY_STABLE041_GEN2_83_STAGE_ABSENT_NO_GPU_OR_WRITES\n')
assert hashlib.sha256((here / 'pioneer_native_dram_load64_physical.sh').read_bytes()).hexdigest() == 'bcee28ab0caa808a4f662c1803980de221892ebcb1ba5998d8137ac01a10456b'
assert hashlib.sha256((here / 'pioneer_native_dram_load_physical.sh').read_bytes()).hexdigest() == 'f6859f95a58524cca324e0f150b358c63d2656e8af12eb3b71af1582726a1ba9'
assert '89419 frozen83 SDK+image+private lease+actual native fixture group CLOSED0' in text('terminal-status.txt')
assert 'No83 staging/hardware execution' in text('terminal-status.txt')
print('PASS52 immutable C64-control83 preparation receipts:actual SDK/VM/private lease/native/client/parent,82/81 controls BYTE IDENTICAL;stable041/64CPU/fullRAM preserved;NO83 hardware/byte/FPU/full-model/speed proof')
