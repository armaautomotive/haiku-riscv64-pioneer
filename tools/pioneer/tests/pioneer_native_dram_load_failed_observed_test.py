#!/usr/bin/env python3
"""Immutable actual FAILED82 pre-reset bytes and fresh-epoch recovery."""
import hashlib
import pathlib
import re
from pioneer_native_dram_load_output import BANNERS, verify

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-load-failed-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
assert len(records) == 81 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
assert text('physical-parent.exit') == '70\n' and text('load.exit') == '1\n'
assert text('state') == 'UNPROVEN_LOAD_ONLY_NO_FURTHER_END_RESET_RETRY_RESTORE\n'
assert text('load.stdout') == '\n'.join(BANNERS) + '\n'
assert records['load.stdout'] == '63f6c0619cb2769932daf9d8b83d644076a70c953a867ab061c6135ec597d99d'
assert records['load.stderr'] == 'f21f376681d8ab635a7df556cc6f231f3ba52073f92668c27a3922981094da7e'
assert text('load.stderr') == ('NATIVE_LOAD_ONLY_END_UNPROVEN state4 cold1 phase5 marker314c444e count80001400 '
    'payload_checked272 headers_checked16 mismatch_kind2 mismatch_offset90460;'
    'CLOSE_ONLY_NO_END_RETRY_RESET_RESTORE;NO_PRIVATE_WORD_DUMP\n')
assert 0x90000 + 1120 == 0x90460 and 272 * 4 == 1088
assert records['physical-parent.log'] == 'a36ce019d8e685c02e82712c95ff2edce6a60d91cb5e1cc45a1603af9f136215'
assert len((folder / 'physical-parent.log').read_bytes()) == 1806
outer = text('physical-parent.log')
assert outer.endswith('LOAD_ONLY_STOP_RETAIN_OWNER_NO_RETRY\n') and 'PASS_NATIVE' not in outer
assert 'tag2026110400010082 generation3 quiesced0 cold1 active_owner1 open_count1' in text('physical-collect.log')
for name in ('load.stdout', 'load.stderr'):
    assert records[name] in text('physical-collect.log')  # Independent remote/local hashes.
try:
    verify(text('load.stdout'))
except ValueError:
    pass
else:
    raise AssertionError('Actual failed82 record promoted')
exceptions = {'stage-copy.stderr': 'subsystem request failed on channel 0',
              'recovery-off-evidence.stderr': 'Operation timed out',
              'linux-readonly.stderr': 'Operation timed out',
              'linux-readonly-r2.stderr': 'Operation timed out',
              'haiku-fresh-readonly.stderr': 'Operation timed out',
              'linux-prepare.stderr': 'Warning: No cmdline or append string provided',
              'load.stderr': 'NATIVE_LOAD_ONLY_END_UNPROVEN',
              'cli-refusal.stderr': 'Use --fixed-load-only-band'}
for name in records:
    if name.endswith('.stderr'):
        if name in exceptions:
            assert exceptions[name] in text(name)
        else:
            assert not text(name), name
assert 'EMPTY_PRIVATE_STAGE_NO_COPY_OR_DRIVER_CHANGE' in text('stage-empty.log')
times = []
for suffix in ('r2', 'r3', 'r4'):
    snap = text('progress-cpu-' + suffix + '.log')
    match = re.search(r'^pioneer-native-dram-load-diagno\s+685\s+(?:run|wait)\s+10\s+(\d+)\s+(\d+)\s*$', snap, re.M)
    assert match
    times.append(int(match[1]))
assert times == sorted(set(times)) and times[0] == 250685 and times[-1] == 391143
assert text('recovery-off-once.log').count('action complete: off') == 1
assert 'for 7 seconds' in text('recovery-off-once.log')
assert text('recovery-on-once.log').count('action complete: on') == 1
assert 'for 0.75 seconds' in text('recovery-on-once.log')
assert text('linux-readonly-r3.log').startswith('Linux 6.1.31 riscv64\n77.19 ')
assert '\nmilkv\n0\n' in text('linux-readonly-r3.log')
assert '9cfc6dc326c938b8d611c8c8622a9a6686ed530270dde768b1f01362f4f61f6e' in text('linux-readonly-r3.log')
assert 'PREPARED load82-recovery-20261104-a with 64 CPUs (memory128-spin-idle7-top-page-nosd-control)' in text('linux-prepare.log')
assert text('linux-boot-haiku.log').endswith('Switching to Haiku via normal Linux shutdown; default NVMe boot unchanged\n')
assert 'generation1 quiesced0 cold0 active_owner0' in text('haiku-fresh-readonly-r2.log')
assert '18 first-failure flow+8 metadata+11 recorded-failure/prior cases' in text('recovery-test-final.log')
restore = text('recovery-restore.log')
assert restore.count('QUIESCED_CLOSED;') == 1
assert restore.endswith('NEW_POWER_EPOCH_STABLE041_RESTORED_NO_GPU_EXECUTION\n')
final = text('recovery-final-readonly.log')
assert final.startswith('P100A_RELOAD driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n')
for pin in ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
            '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
            'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
            '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933'):
    assert pin in final
assert '64 (Unknown)' in final and '/ 137034936320)' in final and 'PROJECT_COMPUTE_PARENT_COUNT 0' in final
assert 'ORIGINAL_METADATA 0:1:600:18973870528' in final
assert 'CACHE_ENTRY_FILE_COUNT_851_METADATA_ONLY_NOT_CONTENT_REHASH' in final
assert final.endswith('READONLY_CLEAN041_GEN2_64CPU_FULLRAM_CACHE_ENABLED_WORKER_UNCHANGED_FAILED_TRIAL_PRESERVED_NO_GPU_OR_WRITES\n')
assert hashlib.sha256((here / 'pioneer_native_dram_load_physical.sh').read_bytes()).hexdigest() == 'f6859f95a58524cca324e0f150b358c63d2656e8af12eb3b71af1582726a1ba9'
assert hashlib.sha256((here / 'pioneer_native_dram_load_recovery_restore.sh').read_bytes()).hexdigest() == '4ee287558106aade3ce8d016b85d93db5c18d3fa48cce5818a0481f51d68eb41'
assert '30639 physical-parent CLOSED70' in text('terminal-status.txt')
assert '20076 final independent readonly CLOSED0' in text('terminal-status.txt')
print('PASS81 immutable FAILED82/recovery receipts:tile0 exact,tile1 first-word mismatch BEFORE reset/NO FPU;no byte/math/model/speed pass;single OFF/ON,fresh041/gen2/64CPU/fullRAM/cache preserved')
