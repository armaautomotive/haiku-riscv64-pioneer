#!/usr/bin/env python3
"""Posthoc DEBUG74 or explicit DEBUG75 generation DATA vs pinned CPU controls.

No device, upload, model inference, Work import or execution authority. This
does NOT compare all hidden/vocabulary values or prove the log's provenance.
"""
import argparse
import os
import pathlib
import re
import stat
import sys

from compare_pioneer_q38_observed_trace import load_cpu, require

HEADER = 'DEBUG74_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NOT_ASSISTANT_WORK_OR_TOKEN_RATE'
OPEN = 'DEBUG74_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT'
END = ('PASS_DEBUG74 uploaded_parts3551 pages4736090 spans20367 records3 clean_end1;'
       'DATA_CONSISTENCY_ONLY_EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_TOKEN_RATE')
TOTAL_BYTES = 4736090*4096
HEADER75 = 'DEBUG75_GENERATION_FIXED_2PROMPT_3POSITION_DIAGNOSTIC_NO_CPU_MODEL_REFERENCE_NOT_ASSISTANT_WORK_OR_TOKEN_RATE'
OPEN75 = 'DEBUG75_GENERATION_OPEN_AFTER_FULL_SAME_OPEN_GGUF_SHA_AND_COMPLETE_851_LAYOUT'
START75 = 'DEBUG75_GENERATION_START_ACKNOWLEDGED;NOT_COMPLETED_POSITION_OR_TOKEN'
TERMINAL75 = 'DEBUG75_GENERATION_TERMINAL_READY_BEGIN_HOLD_DRAIN_AND_RESTORE;NOT_CLEAN_END_YET'
END75 = ('PASS_DEBUG75_GENERATION uploaded_parts3551 pages4736090 spans20367 records3 clean_end1;'
         'DATA_CONSISTENCY_ONLY_EXTERNAL_NUMERICAL_COMPARISON_REQUIRED_NOT_STEADY_TOKEN_RATE')

def records_from_log(path, generation_tag75=False):
    with path.open('rb') as source:
        before = os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and 0 < before.st_size <= 256*1024, 'log type/length')
        data = source.read(256*1024+1)
        after = os.fstat(source.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        require(identity(before) == identity(after) and len(data) == before.st_size, 'log changed while reading')
    try:
        lines = data.decode('ascii').splitlines()
    except UnicodeDecodeError:
        raise ValueError('non-ASCII diagnostic log')
    header, opened, ended = (HEADER75, OPEN75, END75) if generation_tag75 else (HEADER, OPEN, END)
    prefix = 'DEBUG75_GENERATION' if generation_tag75 else 'DEBUG74'
    require(len(lines) >= 28+2*generation_tag75 and lines[:2] == [header, opened] and lines[-1] == ended, 'not a complete fixed selected-family diagnostic')
    records, uploaded, at = [[0]*64 for _ in range(3)], 0, 2
    while at < len(lines)-1 and lines[at].startswith(prefix+'_UPLOADED '):
        match = re.fullmatch(re.escape(prefix)+r'_UPLOADED ([0-9]+)/([0-9]+)', lines[at])
        require(match is not None, 'malformed upload progress')
        done, total = map(int, match.groups())
        require(total == TOTAL_BYTES and uploaded < done <= total and done%4096 == 0, 'upload progress range/order')
        uploaded = done
        at += 1
    if generation_tag75:
        require(lines[at:at+2] == [START75, TERMINAL75], 'missing/altered/duplicate generation phase DATA')
        at += 2
    require(uploaded == TOTAL_BYTES and len(lines)-at == 25, 'incomplete upload or record count')
    for pos in range(3):
        for row in range(8):
            match = re.fullmatch(re.escape(prefix)+r'_RECORD position([012]) word([0-9]{2})((?: [0-9a-f]{8}){8})', lines[at])
            require(match is not None and int(match[1]) == pos and int(match[2]) == row*8, 'record row/order/encoding')
            records[pos][row*8:row*8+8] = [int(word, 16) for word in match[3].split()]
            at += 1
    for pos, r in enumerate(records):
        expected_input = 248319 if pos == 0 else 17 if pos == 1 else records[1][4]
        require(r[:4] == [0x31435451, 1, pos, expected_input] and r[4] < 248320
                and r[5]&0x7f800000 != 0x7f800000, 'record identity/input/finite score')
        require(r[6:12] == [pos+1, (pos+1)&1, int(pos == 0), int(pos < 2), 64, 973], 'record completion flags')
        require(r[12:36] == records[0][12:36] and r[38] == r[42]
                and r[44:47] == [pos+1, 48, 119] and not any(r[47:]), 'record session/counter alias/reserved data')
    return records

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('log', type=pathlib.Path)
    parser.add_argument('--generation-tag75', action='store_true', help='Require the distinct complete fixed tag75 generation log;never reinterpret old74 or a boundary snapshot')
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.log.is_absolute(), 'absolute paths required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    cpu, actual = load_cpu(args.cpu), records_from_log(args.log,args.generation_tag75)
    exact = True
    for pos, (reference, record) in enumerate(zip(cpu, actual)):
        h = reference['header']
        same_input, same_token, same_score = record[3] == h[3], record[4] == h[4], record[5] == h[6]
        exact &= same_input and same_token and same_score
        family = 'DEBUG75_GENERATION' if args.generation_tag75 else 'DEBUG74'
        print(f'COMPARE_{family} position{pos}/sameInput{int(same_input)}/CPUgreedy{h[4]}/observedGreedy{record[4]}'
              f'/selectedMatch{int(same_token)}/maxLogitBitsMatch{int(same_score)}')
    print(f'RECORD_DATA_GREEDY_AND_MAXLOGIT_BITEXACT={int(exact)} NO_TOLERANCE_USED;'
          'NOT full-hidden/vocabulary/native-lifetime/log-provenance/physical/Work/TPS qualification')
    return 0 if exact else 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print(f'DEBUG74_RECORD_REFUSAL {error}', file=sys.stderr)
        sys.exit(2)
