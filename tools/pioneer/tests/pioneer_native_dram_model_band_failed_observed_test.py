#!/usr/bin/env python3
"""Immutable actual FAILED81 result and NEW-epoch recovery,never a GPU pass."""
import hashlib
import math
import pathlib
import re
import struct
from pioneer_native_dram_model_band_output import BANNERS, reference, verify

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-band-failed-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
assert len(records) == 61 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
assert text('physical-parent.exit') == '70\n' and text('gddr.exit') == '1\n'
assert text('state') == 'UNPROVEN_GDDR_BANDS_NO_END_RESET_RETRY_RESTORE\n'
assert text('gddr.stdout') == '\n'.join(BANNERS) + '\n'
assert not text('physical-parent.stderr') and len(text('physical-parent.log').encode()) == 7279
assert records['physical-parent.log'] == 'afa8e0dd62bebe1b4dc69712627993eb6a2cad59417f68399f5b2ce727e6c994'
assert records['gddr.stdout'] == '8c864eaa071bb226406bc5b10b96fea1e826b04db6333a1cc5a9c007bb9f7d36'
assert records['gddr.stderr'] == 'c853295ad73f590a350937b721fe97ce754fb229ecf73b4da17f17e5f5c37e05'
error = text('gddr.stderr')
assert error.startswith('NATIVE_GDDR_NUMERICAL_MISMATCH sample0 pass0 row0 got3e4023c0 expected3f0795e8;CLOSE_ONLY_NO_END_OR_RETRY\n')
expected = reference();words = {}
for line in error.splitlines()[1:]:
    m = re.fullmatch(r'NATIVE_GDDR_BAND_RETURNED_WORD sample0 first0 pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
    assert m
    job, row = map(int, m.group(1, 2));assert row < 32 and (job, row) not in words
    assert m[4] == expected[0, row] and m[3] != m[4]
    assert math.isfinite(struct.unpack('<f', struct.pack('<I', int(m[3], 16)))[0])
    words[job, row] = m[3]
assert len(words) == 64 and all(words[0, row] == words[1, row] for row in range(32))
outer = text('physical-parent.log')
assert 'PASS_NATIVE_GDDR' not in outer and 'sample1' not in outer and 'sample2' not in outer
assert outer.endswith('NATIVE_GDDR_STOP_RETAIN_OWNER_NO_RETRY\n')
assert 'generation11 quiesced0 cold1 active_owner1 open_count1' in text('physical-collect.log')
try:
    verify(text('gddr.stdout'))
except ValueError:
    pass
else:
    raise AssertionError('Actual failed81 output was promoted')
for stem in ('admission-readonly', 'stage-create', 'stage-copy', 'stage-postflight', 'progress-readonly',
             'physical-collect', 'physical-fetch', 'recovery-sync', 'recovery-off-once', 'recovery-on-once',
             'linux-helper-hash', 'linux-boot-haiku', 'haiku-fresh-readonly', 'recovery-copy',
             'recovery-script-postflight', 'recovery-restore', 'recovery-final-readonly', 'recovery-test', 'output-parser'):
    assert not text(stem + '.stderr')
assert 'Operation timed out' in text('recovery-off-evidence.stderr')
assert 'Operation timed out' in text('linux-readonly.stderr')
assert 'Permission denied' in text('linux-readonly-r2.stderr')
assert text('linux-prepare.stderr') == 'Warning: No cmdline or append string provided\n'
assert text('recovery-off-once.log').count('action complete: off') == 1
assert text('recovery-on-once.log').count('action complete: on') == 1
assert 'PREPARED gddr81-recovery-20261104-a with 64 CPUs (memory128-spin-idle7-top-page-nosd-control)' in text('linux-prepare.log')
assert 'Switching to Haiku via normal Linux shutdown; default NVMe boot unchanged' in text('linux-boot-haiku.log')
assert 'generation1 quiesced0 cold0 active_owner0' in text('haiku-fresh-readonly.log')
assert '18 local first-failure flow+8 epoch/owner/cold metadata cases' in text('recovery-test.log')
assert '1 valid/622 refused' in text('output-parser.log')
assert text('recovery-restore.log').endswith('NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION\n')
final = text('recovery-final-readonly.log')
assert '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403' in final
assert 'generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' in final
assert '64 (Unknown)' in final and '/ 137034936320)' in final and 'PROJECT_COMPUTE_PARENT_COUNT 0' in final
assert 'PRESERVED_ORIGINAL_FAILED_STATE UNPROVEN_GDDR_BANDS_NO_END_RESET_RETRY_RESTORE' in final
assert 'CACHE_ENTRY_FILE_COUNT_851_METADATA_ONLY_NOT_CONTENT_REHASH' in final
assert final.endswith('READONLY_CLEAN041_GEN2_64CPU_FULLRAM_CACHE_ENABLED_WORKER_UNCHANGED_FAILED_TRIAL_PRESERVED_NO_GPU_OR_WRITES\n')
assert hashlib.sha256((here / 'pioneer_native_dram_model_band_physical.sh').read_bytes()).hexdigest() == '577aa3cc051b036f815a94e519b98489fbc6793a4c0e47ee78e7f8539cc4650a'
assert hashlib.sha256((here / 'pioneer_native_dram_model_band_recovery_restore.sh').read_bytes()).hexdigest() == '17f26da89df54a9fa1db5ec1bebc624e382858aeac321bc3ccce2f1d1fa0e9d6'
print('PASS61 immutable FAILED81/recovery receipts:actual parent70/child1/64 mismatches/no END,next samples or numerical pass;single OFF/ON,new epoch041/gen2/64CPU/fullRAM/cache preserved;NO speed/full-native proof')
