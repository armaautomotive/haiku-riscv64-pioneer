#!/usr/bin/env python3
"""Immutable OFFBOARD binding receipts,not device/inference/performance evidence."""
import hashlib
import pathlib
import re

assert __debug__, 'Assertions required'
folder = pathlib.Path(__file__).resolve().parents[1] / 'benchmarks/qwen38-native-dram-binding-offboard-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and name not in records and pathlib.Path(name).name == name
    path = folder / name
    assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest() == digest
    assert path.suffix in ('.log', '.stderr', '.sh', '.txt', '.disassembly'), 'No generated weight data/binary export'
    records[name] = digest
assert len(records) == 40 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for stem in ('native-build-r2', 'native-build-r3', 'native-build-r4', 'native-build-r5',
             'native-endpoints-r1', 'native-endpoints-r3', 'native-endpoints-r4', 'native-endpoints-r5',
             'lease-regression', 'protocol-role-regression', 'lossless-cache-regression', 'weight-cache-regression',
             'admission-readonly', 'admission-readonly-r2'):
    assert not text(stem + '.stderr')
assert 'no member named' in text('native-build.stderr') and 'SweepQuarantineMemoryOnly' in text('native-build.stderr')
assert 'Assertion failed:' in text('native-endpoints-r2.stderr') and 'clock.TlbRead(0)' in text('native-endpoints-r2.stderr')
final = text('native-build-r5.log')
assert '0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27 tenstorrent_pci_probe-native-dram-model-band81' in final
assert '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc tenstorrent_pci_probe-default43-control' in final
assert 'cd924896986dabcead8493c9dedca1d7b25fde323fbd972caf67ad93139a9309 4436 text.bin' in final
assert 'actual driver prologue14 forbidden' in final and 'NO install/endpoint tests/hardware/full-model/speed authority' in final
assert len(text('driver-role-refusals.txt').splitlines()) == 14
assert 'valid307 refused643' in text('native-endpoints-r5.log') and 'NO native ARC/prerequisite/engine/hardware/full-model/speed proof' in text('native-endpoints-r5.log')
assert len(text('native-source-sha256.txt').splitlines()) == 6
lease = text('lease-regression.log')
for sample in range(3):
    assert 'sample' + str(sample) + ':exact64 rounded words' in lease
assert '35 failure/admission/close cases' in lease and '3 forbidden production roles' in lease
assert 'roles10 cases,protocol352 dirty-result-byte refusals+old-tag refusal,exact392B ABI' in text('protocol-role-regression.log')
assert '61 checks,27247 exact upload pages' in text('lossless-cache-regression.log')
assert '45 checks;0 unsupported filesystem fixtures' in text('weight-cache-regression.log')
health = text('admission-readonly-r2.log')
assert 'generation10 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' in health
assert '64 (Unknown)' in health and '/ 137034936320)' in health and 'PROJECT_COMPUTE_PARENT_COUNT 0' in health
assert '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74' in health
assert 'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18' in health
assert health.endswith('READONLY_CLEAN041_GEN10_64CPU_FULLRAM_CACHE_ENABLED_WORKER_UNCHANGED_NO_GPU_OR_WRITES\n')
assert 'READONLY_CLEAN' not in text('admission-readonly.log'), 'Earlier parser failure cannot be promoted'
stack = text('native-stack.txt')
assert re.search(r'PioneerNativeDram81::Control[^\n]*\t672\tstatic', stack)
assert re.search(r'PioneerNativeDramModelBandLease::Run[^\n]*\t192\tstatic', stack)
print('PASS40 immutable OFFBOARD native81 receipts;actual link/307 valid+643 refusal fixtures/cache regressions/clean readonly machine;old failures preserved;NO native firmware/hardware/full-model/speed proof')
