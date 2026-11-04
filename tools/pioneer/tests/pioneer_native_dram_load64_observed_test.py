#!/usr/bin/env python3
"""Immutable ACTUAL83 transfer/closure,not FPU/model/speed qualification."""
import hashlib
import pathlib
import re
from pioneer_native_dram_load_output import verify

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-load64-observed-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
stems = ('admission-stage-create', 'stage-copy', 'stage-postflight', 'physical-parent',
         'progress-cpu', 'progress-cpu-r2', 'physical-collect', 'physical-fetch',
         'final-readonly', 'final-readonly-r2', 'final-readonly-r3', 'output-parser')
expected = {stem + suffix for stem in stems for suffix in ('.log', '.stderr')}
expected.update(('load.stdout', 'load.stderr', 'load.exit', 'state', 'parent.pid',
                 'cli-refusal.stdout', 'cli-refusal.stderr', 'cli-refusal.exit',
                 'terminal-status.txt', 'physical-parent.exit'))
assert len(records) == 34 and set(records) == expected
assert set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}

def text(name):
    assert name in records
    return (folder / name).read_text()

for name in records:
    if name.endswith('.stderr') and name != 'cli-refusal.stderr':
        assert not text(name), name
assert text('load.exit') == text('physical-parent.exit') == '0\n'
assert text('state') == 'STABLE041_RESTORED_LOAD64_ONLY_PAYLOAD_AND_HEADERS_PASSED\n'
assert text('cli-refusal.exit') == '2\n' and not text('cli-refusal.stdout')
assert text('cli-refusal.stderr') == ('Use --fixed-load64-only-band for one fixed DEBUG C64 '
    'placement control,NO FPU/model/Work inference.\n')
assert records['load.stdout'] == 'a8884f789db23da897578ffb73a1c0849fa8e1e972f6e7701d10647b51df698c'
assert len((folder / 'load.stdout').read_bytes()) == 442
assert verify(text('load.stdout'), True) == 1
for raw, role in ((text('load.stdout'), False),
                  ((here / 'benchmarks/qwen38-native-dram-load-failed-20261104/load.stdout').read_text(), True)):
    try:
        verify(raw, role)
    except ValueError:
        pass
    else:
        raise AssertionError('Cross-role/failed record promoted')
outer = text('physical-parent.log')
assert records['physical-parent.log'] == 'b4b8edd29c1d4fad9ad8bb2dd0c9a7b837addce1d7703adc523d198f039e1264'
assert len((folder / 'physical-parent.log').read_bytes()) == 3183
assert outer.count(text('load.stdout')) == 1 and outer.count('PASS_NATIVE_DRAM_LOAD64_ONLY') == 1
assert outer.count('HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION') == 2
assert outer.count('QUIESCED_CLOSED;') == 2
stable = 'P100A_RELOAD driver55 tag2026102100010012 generation%d quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n'
candidate = 'P100A_RELOAD driver55 tag2026110400010083 generation3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n'
assert outer.startswith(stable % 2) and candidate in outer and stable % 4 in outer
assert outer.index(candidate) < outer.index('AUTHENTICATING_FIXED') < outer.index('PASS_NATIVE') < outer.index(stable % 4)
assert outer.endswith('LOAD64_ONLY_KNOWN_CLEAN_NO_FPU_MODEL_OR_SPEED_CLAIM\n')
collect = text('physical-collect.log')
for name in ('load.stdout', 'load.stderr', 'load.exit', 'state', 'cli-refusal.stdout',
             'cli-refusal.stderr', 'cli-refusal.exit', 'parent.pid'):
    assert records[name] in collect  # Remote/local independently fetched bytes match.
assert text('state') in collect and stable % 4 in collect
assert text('output-parser.log') == 'ACTUAL83_INDEPENDENT_PARSE polls 1\n'
pins = ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
        '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
        'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
        '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933')
for name in ('physical-collect.log', 'final-readonly.log', 'final-readonly-r3.log'):
    final = text(name)
    assert stable % 4 in final and '64 (Unknown)' in final and '/ 137034936320)' in final
    assert all(pin in final for pin in pins)
    assert text('state') in final
final = text('final-readonly.log')
assert 'PROJECT_COMPUTE_PARENT_COUNT 0' in final
assert 'PRESERVED_PRIOR82_FAILURE_NOT_RECLASSIFIED' in final
assert 'ORIGINAL_METADATA 0:1:600:18973870528' in final
assert 'CACHE_ENTRY_FILE_COUNT_851_METADATA_ONLY_NOT_CONTENT_REHASH' in final
assert final.endswith('READONLY_CLEAN041_GEN4_64CPU_FULLRAM_CACHE_PROGRAMS_UNCHANGED_C64_TRANSFER_CLOSED_NO_EXTRA_GPU_OR_WRITES\n')
assert not text('final-readonly-r2.log')  # Unsupported option refused;NOT evidence of health.
assert text('final-readonly-r3.log').endswith('READONLY_OBSERVATION_ONLY_NO_DEVICE_EXECUTION_OR_WRITES\n')
assert '133784285184 bytes free' in text('final-readonly-r3.log')
times = []
for name, size in (('progress-cpu.log', 92), ('progress-cpu-r2.log', 186)):
    snap = text(name)
    match = re.search(r'^pioneer-native-dram-load64-diag\s+669\s+run\s+10\s+(\d+)\s+(\d+)\s*$', snap, re.M)
    assert match and 'load.stdout:%d' % size in snap and 'load.stderr:0' in snap
    assert 'PASS_NATIVE' not in snap
    times.append(int(match[1]))
assert times == [274417, 462290]
stage = text('stage-postflight.log')
for pin in ('7ee39a3615bc93e3b81cd21149fdd020f2e5f3e7a7f1b3db8cca24bffd94a85e',
            '10310865e07c34b476632e8e776f76fae603a7a4adea2ec87f226f1f05798ac0',
            'bcee28ab0caa808a4f662c1803980de221892ebcb1ba5998d8137ac01a10456b'):
    assert pin in stage
assert stage.endswith('EXACT83_PRIVATE_STAGE_CLI_REFUSED2_NO_DEVICE_OR_DRIVER_CHANGE\n')
assert text('admission-stage-create.log').endswith('FRESH_STABLE041_GEN2_PRIVATE83_STAGE_CREATED_NO_GPU_OR_DRIVER_CHANGE\n')
assert hashlib.sha256((here / 'pioneer_native_dram_load64_physical.sh').read_bytes()).hexdigest() == 'bcee28ab0caa808a4f662c1803980de221892ebcb1ba5998d8137ac01a10456b'
assert hashlib.sha256((here / 'pioneer_native_dram_load_physical.sh').read_bytes()).hexdigest() == 'f6859f95a58524cca324e0f150b358c63d2656e8af12eb3b71af1582726a1ba9'
assert '17286 ONE physical parent ACTUALLY CLOSED0' in text('terminal-status.txt')
assert '314fd0 documented --status independent final readonly CLOSED0' in text('terminal-status.txt')
print('PASS34 immutable ACTUAL83 C64 transfer/closure receipts:all46080 payload+5120 headers;normal041/gen4/64CPU/fullRAM/cache preserved;old failures NOT promoted;NO FPU/model/speed proof')
