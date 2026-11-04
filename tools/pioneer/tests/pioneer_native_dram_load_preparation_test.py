#!/usr/bin/env python3
"""Immutable offboard82 preparation receipts;NEVER physical/model proof."""
import hashlib
import pathlib
import re

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-load-preparation-20261104'
records = {}
for line in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = line.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
names = {'terminal-status.txt', 'source-SHA256SUMS', 'driver-role-refusals.txt',
         'native-legacy-entry.disassembly', 'native-exports.txt'}
for stem in ('image-test', 'lease-test', 'native-endpoints', 'math81-endpoints',
             'client-test', 'parent-test', 'native-build', 'math81-build', 'preflight'):
    names.update((stem + '.log', stem + '.stderr'))
assert set(records) == names == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for name in names:
    if name.endswith('.stderr'):
        assert not text(name)
for name, needle in (
        ('image-test.log', '8 route-shape valid/21 frame/NIU/FIFO/counter/header/job faults'),
        ('lease-test.log', '90 mismatch/failure/admission cases'),
        ('native-endpoints.log', 'valid308 refused665'),
        ('math81-endpoints.log', 'valid307 refused643'),
        ('client-test.log', 'client:228 ASan/UBSan cases'),
        ('parent-test.log', '27 flow/output+7 idle+11 failed-prior/recovery/model+20 exchange-step+10 one-shot cases')):
    assert needle in text(name)
lease = text('lease-test.log')
for sample in range(3):
    assert lease.count('PASS OFFBOARD actual LOAD ONLY sample%d:' % sample) == 1
assert lease.count('46080 payload+2560 header words exact') == 3
image = '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'
new_driver = '3df4fdb1d034c39df5c24bc06735012109d3881431c5a9ddaedd16ecc535d3d6'
client = 'f7ae6e1be5fbcf6618ddd4326715c49166169a0073f868a86ec353519bc4f719'
default = '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
build = text('native-build.log')
for pin in (image, new_driver, client, default):
    assert pin in build
assert 'LINKED/15 forbidden roles/legacy NULL/default unchanged' in build
old = text('math81-build.log')
for pin in ('0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27',
            '0091244ae265e26075e80fa98f5d6b8b5c6d37304938e3d140d9bab6584e307e', default):
    assert pin in old
assert len(text('driver-role-refusals.txt').splitlines()) == 15
preflight = text('preflight.log')
assert preflight.startswith('Haiku\nP100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n')
assert '64 (Unknown)' in preflight and '/ 137034936320)' in preflight
for pin in ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
            '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
            'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18'):
    assert pin in preflight
status = text('terminal-status.txt')
for handle in ('90592', '26139', '8030', '62760', '26756', '45032', 'c1d835'):
    assert re.search(r'^' + handle + r' .+ CLOSED0$', status, re.M)
assert 'NOT physical byte/math/model proof' in status
assert hashlib.sha256((here / 'pioneer_native_dram_load_physical.sh').read_bytes()).hexdigest() == 'f6859f95a58524cca324e0f150b358c63d2656e8af12eb3b71af1582726a1ba9'
source_pins = {}
for line in text('source-SHA256SUMS').splitlines():
    digest, path = line.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and path not in source_pins
    assert not pathlib.Path(path).is_absolute() and '..' not in pathlib.Path(path).parts
    source_pins[path] = digest  # Historical build-source tuple,not future code authority.
assert len(source_pins) == 20
print('PASS23 immutable LOAD-ONLY82 preparation receipts:compiled RV32/private-data lease/native/client/parent/old81 regression;stable041/gen2/64CPU/fullRAM/cache-worker untouched;NO82 physical byte/FPU/model/speed proof')
