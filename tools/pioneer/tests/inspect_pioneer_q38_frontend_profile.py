#!/usr/bin/env python3
"""Strict raw profile DATA parsing and address bins,NOT retired-PC/cycle proof."""
import argparse
import collections
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import tempfile
import sys
import compare_pioneer_q38_frontier_benchmark_log as base
from compare_pioneer_q38_observed_trace import load_cpu, require

HEADER = 'Q38_FRONTEND_PROFILE_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_NOT_RETIRED_PC_OR_CYCLES'
BEGIN = re.compile(r'Q38_FRONTEND_ROWS_BEGIN generation([0-9]+) rows([0-9]+);AFTER_CLEAN_END_NOT_ATOMIC_WITH_PRECEDING_COUNT')
ROW = re.compile(r'Q38_FRONTEND_ROW index([0-9]+) begin_us([0-9]+) end_us([0-9]+) preceding_count([0-9]+) ready([0-9]+) stage([0-9]+) checked_word([0-9]+) first_raw([0-9a-f]{8}) second_raw([0-9a-f]{8})')
END = re.compile(r'Q38_FRONTEND_ROWS_END rows([0-9]+);FRONTEND_SAMPLE_COUNTS_NOT_RETIRED_INSTRUCTION_CYCLES_OR_FUNCTION_TIME')
# These are caller-declared bindings to independently build-qualified modules,
# NOT an in-band cryptographic proof of the hardware's installed module.
BINDINGS = {
 '9ad9e86bd5e45c5cb11dfe286f3528452dfe7b0e4630f67546acbb43a03064e8':
  (190684, '9515589d211cb9d9b2db4be108ccad5752996ec07a5354f0181775f6a6888862'),
 '1f3bcff838ff9f95d5f450c53000f02f80ddcd073aa5a5cc3018ee4208e0847c':
  (190700, '48b2bf31d2eefc36e50dcaaa9202d7d92579496a5ab72e9a62c9d4cfb66250ee'),
 '78322cfa94db2519284efa12ad69cfa77981446f03081fc95aee2ee6234de8d1':
  (190704, '83677f78f548b8fc11d874a858b670310d1dad859982e737c674b2ff7329db00'),
 'e09cc07492d201b6a23f454ae0be514597051f8c55cf143b518eeb0b2f6c5e35':
  (190952, 'bc01b290d588c3809acc22288fedbc5445ce1d152aa2a39fb86b496437a8bbd2'),
 '0b8d1530b3f6de3462be23e923be169c82a8b15c76e83657c35c2d268b70405c':
  (191408, 'a2d52546a4606b37c6cb421ac412a8d3eae59135492edecbc1a96079d24af536'),
 # Distinct qualified native modules share the SAME immutable ROOT ELF.
 # The prefetch change is in peer firmware,not a new ROOT address map.
 '8612962e38cab47999b8aecf67268c8e7fd651917b8a570fd79f79b78e1b165f':
  (191436, '88e419152d297dbe2ebb0b3913d3e2cae58aa69741b4b03e2667749702c4a674'),
 '919d4a631ff664549a67bb2372ce2db344ba29473c452c0b273c4b27a2bdf978':
  (191436, '88e419152d297dbe2ebb0b3913d3e2cae58aa69741b4b03e2667749702c4a674'),
}


def stable(path, limit):
    require(path.is_absolute() and not path.is_symlink(), 'absolute regular nonsymlink file required')
    with path.open('rb') as source:
        before = os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and 0 < before.st_size <= limit, 'profile/ELF type or length')
        data = source.read(limit+1)
        after = os.fstat(source.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        require(identity(before) == identity(after) and len(data) == before.st_size, 'changed file')
    return data


def read_profile(path):
    try:
        lines = stable(path, 8*1024*1024).decode('ascii').splitlines()
    except UnicodeDecodeError:
        raise ValueError('non-ASCII profile')
    require(lines and lines[0] == HEADER, 'not the distinct frontend profile mode')
    starts = [i for i, line in enumerate(lines) if BEGIN.fullmatch(line)]
    require(len(starts) == 1, 'missing/duplicate profile rows begin')
    at = starts[0]
    begin, end = BEGIN.fullmatch(lines[at]), END.fullmatch(lines[-1])
    require(end is not None, 'missing rows end')
    generation, count = map(int, begin.groups())
    require(0 < generation < 1 << 64 and 1 <= count <= 18000 and int(end[1]) == count,
            'generation/count range')
    require(len(lines)-at == count+2, 'profile row count')
    rows = []
    for index, line in enumerate(lines[at+1:-1]):
        match = ROW.fullmatch(line)
        require(match is not None, 'raw row grammar')
        i, first, last, preceding, ready, stage, word = map(int, match.groups()[:7])
        pc1, pc2 = (int(value, 16) for value in match.groups()[7:])
        require(i == index and 0 <= first <= last <= (1 << 63)-1 and preceding <= 34,
                'row order/time/count')
        require(ready == int(index == count-1) and stage == (11 if ready else 9)
                and word < 1 << 32 and (ready or word <= 63), 'row ready/stage/checked-word')
        if rows:
            require(first >= rows[-1]['end'] and preceding >= rows[-1]['preceding_count'], 'regressing profile')
        rows.append({'begin': first, 'end': last, 'preceding_count': preceding,
                     'ready': ready, 'checked_word': word, 'first_raw': pc1, 'second_raw': pc2})
    # The derived temporary is parser input ONLY,never archived as an observed
    # old-mode physical log. Every remaining base line must pass unchanged.
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-profile-base-', dir='/private/tmp') as temporary:
        derived = pathlib.Path(temporary) / 'DERIVED-PARSER-INPUT-NOT-OBSERVED.log'
        derived.write_text('\n'.join(lines[1:at]) + '\n')
        records, rate = base.read_log(derived)
    if rate:
        require(rate['observations'] == count+1, 'one existing Poll per nonfinal metric observation')
    return generation, rows, records, rate


def authenticated_ranges(elf, declared_driver):
    require(declared_driver in BINDINGS, 'unqualified declared driver binding')
    size, digest = BINDINGS[declared_driver]
    data = stable(elf, size)
    require(len(data) == size and hashlib.sha256(data).hexdigest() == digest, 'ROOT ELF binding mismatch')
    tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    # Decode the exact authenticated snapshot,not a later unchecked open of
    # the caller's path. This private temporary is DATA,never executed.
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-profile-elf-', dir='/private/tmp') as temporary:
        snapshot = pathlib.Path(temporary) / 'authenticated-root.elf'
        snapshot.write_bytes(data)
        table = subprocess.check_output([tool, '-S', '-nC', '--defined-only', str(snapshot)], text=True)
    ranges = []
    for line in table.splitlines():
        fields = line.split(None, 3)
        if len(fields) != 4 or fields[2] not in ('t', 'T', 'w', 'W'):
            continue
        first, length = int(fields[0], 16), int(fields[1], 16)
        if length:
            require((0x40000 <= first < first+length <= 0x60000)
                    or (0x64000 <= first < first+length <= 0x6c000), 'function outside qualified code')
            ranges.append((first, first+length, fields[3]))
    require(ranges, 'no function intervals')
    return ranges


def bins(rows, ranges):
    result = collections.Counter()
    for row in rows:
        for field in ('first_raw', 'second_raw'):
            address = row[field]
            names = sorted(set(name for first, last, name in ranges if first <= address < last))
            result[' | '.join(names) if names else 'UNMAPPED_RAW_ADDRESS'] += 1
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('profile', type=pathlib.Path)
    parser.add_argument('root_elf', type=pathlib.Path)
    parser.add_argument('--declared-driver-sha', required=True)
    args = parser.parse_args(argv)
    require(not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ), 'role override')
    generation, rows, records, rate = read_profile(args.profile)
    ranges = authenticated_ranges(args.root_elf, args.declared_driver_sha)
    print(f'PASS profile grammar/base records+rate arithmetic/generation{generation}/rows{len(rows)};'
          'ELF authenticated to DECLARED module,not independent hardware provenance')
    print('OBSERVED_COUNT_WINDOW ' + str(rate))
    for name, count in bins(rows, ranges).most_common():
        print(f'RAW_FRONTEND_ADDRESS_BIN samples{count} {name}')
    print('Address sample counts include fetch-ahead/waits and boundary ambiguity;NO PC subtraction/range widening;'
          'NOT retired instructions,cycles,function time,bottleneck causality,quality or Work proof')
    return 0 if base.compare_first_three(load_cpu(args.cpu), records) else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print('FRONTEND_PROFILE_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(2)
