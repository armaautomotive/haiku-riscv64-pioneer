#!/usr/bin/env python3
"""Immutable ACTUAL84 C64 learned bands/closure,not full-model/speed proof."""
import hashlib
import pathlib
import re
from pioneer_native_dram_model_band_output import verify

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-band64-observed-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
stems = ('stage-create','stage-copy','stage-postflight','physical-parent',
         'progress-cpu','progress-cpu-r2','progress-cpu-r3','progress-cpu-r4',
         'physical-collect','physical-fetch','output-parser','final-readonly')
expected = {stem + suffix for stem in stems for suffix in ('.log','.stderr')}
expected.update(('gddr64.stdout','gddr64.stderr','gddr64.exit','state','parent.pid',
                 'cli-refusal.stdout','cli-refusal.stderr','cli-refusal.exit',
                 'physical-parent.exit','terminal-status.txt'))
assert len(records) == 34 and set(records) == expected
assert set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for name in records:
    if name.endswith('.stderr') and name != 'cli-refusal.stderr':
        assert not text(name), name
assert text('gddr64.exit') == text('physical-parent.exit') == '0\n'
assert text('state') == 'STABLE041_RESTORED_GDDR64_BANDS_EXACT192_PASSED\n'
assert text('cli-refusal.exit') == '2\n' and not text('cli-refusal.stdout')
assert text('cli-refusal.stderr') == 'Use --fixed-gddr64-learned-bands for three fixed DEBUG C64 samples,NOT model/Work inference.\n'
assert records['gddr64.stdout'] == '8f21e56f15b5465e70f6a916ca5ae44b0ce51c2c584a06e3350e7fcb97c00d63'
raw = (folder / 'gddr64.stdout').read_bytes()
assert len(raw) == 18182 and b'\r' not in raw and len(raw.splitlines()) == 198
assert verify(raw.decode('ascii'), True) == {'samples':3,'words':192,'ends':3,'generation':5,'polls':[1,1,1]}
for value, role in ((text('gddr64.stdout'),False),
                    ((here / 'benchmarks/qwen38-native-dram-band-failed-20261104/gddr.stdout').read_text(),True),
                    ((here / 'benchmarks/qwen38-native-dram-load64-observed-20261104/load.stdout').read_text(),True)):
    try:
        verify(value, role)
    except ValueError:
        pass
    else:
        raise AssertionError('Wrong-role/failed/transfer-only record promoted')
outer = text('physical-parent.log')
assert records['physical-parent.log'] == '5bccf8e1014512d6423069bb99fa5a4b39e0c1e782d7a06d9482f9e7505d6546'
assert len((folder / 'physical-parent.log').read_bytes()) == 20922
assert outer.count(text('gddr64.stdout')) == 1
assert outer.count('PASS_NATIVE_GDDR64_BAND sample') == 3
assert outer.count('PASS_NATIVE_GDDR64_BANDS samples3 exact_words192 clean_ends3;') == 1
assert outer.count('HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION') == 2
assert outer.count('QUIESCED_CLOSED;') == 2
stable = 'P100A_RELOAD driver55 tag2026102100010012 generation%d quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n'
candidate = 'P100A_RELOAD driver55 tag2026110400010084 generation5 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n'
assert outer.startswith(stable % 4) and candidate in outer and stable % 6 in outer
assert outer.index(candidate) < outer.index('AUTHENTICATING_FIXED') < outer.index('PASS_NATIVE_GDDR64_BANDS') < outer.index(stable % 6)
assert outer.endswith('NATIVE_GDDR64_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM\n')
collect = text('physical-collect.log')
for name in ('gddr64.stdout','gddr64.stderr','gddr64.exit','state','cli-refusal.stdout','cli-refusal.stderr','cli-refusal.exit','parent.pid'):
    assert records[name] in collect  # Remote/local independently fetched hashes match.
assert text('state') in collect and stable % 6 in collect
assert text('output-parser.log') == "ACTUAL84_INDEPENDENT_FRACTION_REFERENCE_PARSE {'samples': 3, 'words': 192, 'ends': 3, 'generation': 5, 'polls': [1, 1, 1]}\n"
pins = ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
        '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
        'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
        '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933')
for name in ('physical-collect.log','final-readonly.log'):
    snapshot = text(name)
    assert stable % 6 in snapshot and '64 (Unknown)' in snapshot and '/ 137034936320)' in snapshot
    assert all(pin in snapshot for pin in pins)
final = text('final-readonly.log')
assert '133780238336 bytes free' in final
assert 'PRESERVED_PRIOR81_82_FAILURES_AND83_TRANSFER_PASS_NOT_RECLASSIFIED' in final
assert 'ORIGINAL_METADATA_0:1:600:18973870528' in final
assert 'CACHE_ENTRY_FILE_COUNT_851_METADATA_ONLY_NOT_CONTENT_REHASH' in final
assert final.endswith('READONLY_CLEAN041_GEN6_64CPU_FULLRAM_CACHE_PROGRAMS_UNCHANGED_C64_MATH_CLOSED_NO_EXTRA_GPU_OR_WRITES\n')
self_pid = int(re.search(r'^READONLY_SELF_PID (\d+)$',final,re.M)[1])
teams = re.findall(r'^\s*(\d+)\s+(/.+|kernel_team)\s*$',final,re.M)
assert len(teams) >= 20 and self_pid in {int(pid) for pid,_ in teams}
project = re.compile(r'(^|[/\s])(Assistant|HaikuAssistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|pioneer-native|pioneer-dram|pioneer-assistant-cache|graph-csr-debug|source-check)|pioneer-p100a-(dram|batched-output|wide-gdn|token)')
assert not [(pid,team) for pid,team in teams if int(pid)!=self_pid and project.search(team)]
times = []
for name in ('progress-cpu-r2.log','progress-cpu-r3.log'):
    snapshot = text(name)
    match = re.search(r'^pioneer-native-dram-model-band6\s+953\s+run\s+10\s+(\d+)\s+(\d+)\s*$',snapshot,re.M)
    assert match and 'gddr64.stdout:92' in snapshot and 'gddr64.stderr:0' in snapshot
    times.append(int(match[1]))
assert times == [284967,447887]
assert 'Thread' not in text('progress-cpu.log')  # Initial exact-name filter missed truncated team args.
assert text('progress-cpu-r4.log').startswith(text('state')) and 'gddr64.stdout:18182' in text('progress-cpu-r4.log')
stage = text('stage-postflight.log')
for pin in ('2cb420909e36182df421058853d706b50d7e819357496141eba385787d49cd1e',
            '9aad44e18f6c88e8c8c4e8cddb7234b4a576d3362b16dfb98e5c2fee364e79a3',
            'df6675b40f0e1313bb2c1c93faaff56cb34ee223dd15eef9bfbd0477e1fe5ad8',
            '35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83'):
    assert pin in stage
assert stage.endswith('EXACT_PRIVATE84_STAGE_CLI_REFUSED2_NORMAL041_GEN4_NO_DEVICE_OR_DRIVER_CHANGE\n')
assert text('stage-create.log').endswith('FRESH_NORMAL041_GEN4_NEW_PRIVATE84_STAGE_CREATED_NO_GPU_OR_DRIVER_CHANGE\n')
assert hashlib.sha256((here / 'pioneer_native_dram_model_band64_physical.sh').read_bytes()).hexdigest() == 'df6675b40f0e1313bb2c1c93faaff56cb34ee223dd15eef9bfbd0477e1fe5ad8'
assert '52333 ONE frozen physical parent ACTUALLY CLOSED0 on SAME handle' in text('terminal-status.txt')
assert 'bb67ab final readonly normal041/gen6/64CPU/fullRAM/cache/programs CLOSED0' in text('terminal-status.txt')
print('PASS34 immutable ACTUAL84 C64 learned-FPU receipts:192 exact rounded words/3 clean ENDs;normal041/gen6/64CPU/fullRAM/cache preserved;old failures unchanged;NO full-model/speed proof')
