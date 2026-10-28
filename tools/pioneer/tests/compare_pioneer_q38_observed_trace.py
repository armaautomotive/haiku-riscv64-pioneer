#!/usr/bin/env python3
"""Posthoc CPU-vs-emitted DATA comparison. No device IO,uploads or launch grant."""
import argparse
import array
import hashlib
import math
import os
import pathlib
import stat
import struct
import sys

CPU_PINS = (
    '9323e2451c41e241c9a0391d3905890b662fd73df8542103da229f9b910d6937',
    'dcc88c37599ab6659990f5154da04dcc12b4edea5a38c27359402b75ede4973d',
    '433b776057b9ada57c21bd8b103e29464e6f8758ea7cd4e9865d6e8ff2bff214',
)
HIDDEN_WORDS = 65*5120
VOCAB_WORDS = 248320
TOKEN_MAGIC = 0x31435451

def require(condition, message):
    if not condition:
        raise ValueError(message)

def stable_bytes(path, expected_bytes):
    # Authenticate/decode the SAME byte snapshot from the SAME open regular
    # file,not a hash of one pathname followed by a second unchecked open.
    with path.open('rb') as source:
        before = os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and before.st_size == expected_bytes, f'{path}: type/length')
        data = source.read(expected_bytes+1)
        after = os.fstat(source.fileno())
        identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
        require(identity(before) == identity(after) and len(data) == expected_bytes, f'{path}: changed source')
    return data

def decode_trace(data, position, observed):
    record_words = 64 if observed else 0
    require(len(data) == (16+HIDDEN_WORDS+VOCAB_WORDS+record_words+1)*4, 'trace length')
    words = array.array('I')
    require(words.itemsize == 4, '32-bit word host required')
    words.frombytes(data)
    if sys.byteorder != 'little':
        words.byteswap()
    h = words[:16]
    require(h[0] == (0x31444f51 if observed else 0x31524651) and h[1] == 1 and h[2] == position, 'trace family/version/position')
    require(h[3] < VOCAB_WORDS and h[4] < VOCAB_WORDS and h[5] < VOCAB_WORDS and h[4] != h[5], 'trace token range')
    require(list(h[8:12]) == [65, 5120, VOCAB_WORDS, record_words] and not any(h[12:]), 'trace dimensions/reserved')
    require(words[-1] == (0x454e444f if observed else 0x454e4451), 'trace terminal')
    numeric = data[64:(16+HIDDEN_WORDS+VOCAB_WORDS)*4]
    floats = array.array('f')
    require(floats.itemsize == 4, 'binary32 host required')
    floats.frombytes(numeric)
    if sys.byteorder != 'little':
        floats.byteswap()
    require(all(math.isfinite(x) for x in floats), 'nonfinite numeric trace')
    logits = floats[HIDDEN_WORDS:]
    ranked = sorted(range(VOCAB_WORDS), key=lambda i: (-logits[i], i))[:2]
    require(list(h[4:6]) == ranked, 'trace greedy/runner-up vs actual vocabulary')
    require(h[6] == words[16+HIDDEN_WORDS+h[4]] and h[7] == words[16+HIDDEN_WORDS+h[5]], 'trace top-logit words')
    if position < 2:
        require(h[3] == (248319 if position == 0 else 17), 'trace immutable prompt')
    if observed:
        record = words[16+HIDDEN_WORDS+VOCAB_WORDS:-1]
        expected = [TOKEN_MAGIC, 1, position, h[3], h[4], h[6], position+1, (position+1)&1,
                    int(position+1 < 2), int(position+1 < 3), 64, 973]
        require(list(record[:12]) == expected and list(record[44:47]) == [position+1, 48, 119] and not any(record[47:]), 'committed-token record')
    return {'header': h, 'bits': words[16:16+HIDDEN_WORDS+VOCAB_WORDS], 'floats': floats}

def difference(expected, actual, first, count):
    eb, ab = expected['bits'], actual['bits']
    ef, af = expected['floats'], actual['floats']
    differences = sum(eb[i] != ab[i] for i in range(first, first+count))
    error = math.fsum((float(af[i])-ef[i])**2 for i in range(first, first+count))
    norm = math.fsum(float(ef[i])**2 for i in range(first, first+count))
    maximum = max(abs(float(af[i])-ef[i]) for i in range(first, first+count))
    return differences, maximum, math.sqrt(error/(norm+1e-300))

def load_cpu(directory):
    traces = []
    for position in range(3):
        data = stable_bytes(directory/f'token-{position}.bin', (16+HIDDEN_WORDS+VOCAB_WORDS+1)*4)
        require(hashlib.sha256(data).hexdigest() == CPU_PINS[position], 'CPU trace is not the independent fixed original-model control')
        traces.append(decode_trace(data, position, False))
    require(traces[2]['header'][3] == traces[1]['header'][4], 'CPU greedy feedback path')
    return traces

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cpu', type=pathlib.Path)
    parser.add_argument('observed', type=pathlib.Path)
    parser.add_argument('--diagnostic-only', action='store_true', help='Permit exit0 for a valid report with numeric differences; NEVER a numerical pass')
    args = parser.parse_args(argv)
    require(args.cpu.is_absolute() and args.observed.is_absolute(), 'absolute trace directories required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'unexpected role overrides')
    cpu, observed = load_cpu(args.cpu), []
    for position in range(3):
        data = stable_bytes(args.observed/f'token-{position}.bin', (16+HIDDEN_WORDS+VOCAB_WORDS+64+1)*4)
        observed.append(decode_trace(data, position, True))
    require(observed[2]['header'][3] == observed[1]['header'][4], 'observed greedy feedback path within trace')
    exact = True
    for position in range(3):
        e, a = cpu[position], observed[position]
        path = e['header'][3] == a['header'][3]
        selected = e['header'][4] == a['header'][4]
        exact = exact and path and selected
        print(f'COMPARE position{position}/sameInput{int(path)}/CPUgreedy{e["header"][4]}/observedGreedy{a["header"][4]}/selectedMatch{int(selected)}')
        if not path:
            print('INPUT_PATH_DIVERGED: later numeric comparison is not valid;expected trace is NOT retargeted')
            continue
        for layer in range(65):
            diff, maximum, relative = difference(e, a, layer*5120, 5120)
            exact = exact and diff == 0
            print(f'COMPARE_HIDDEN position{position}/embeddingOrLayer{layer}/words5120/bitDifferences{diff}/maxAbs{maximum:.12g}/relativeL2{relative:.12g}')
        diff, maximum, relative = difference(e, a, HIDDEN_WORDS, VOCAB_WORDS)
        exact = exact and diff == 0
        print(f'COMPARE_VOCAB position{position}/words{VOCAB_WORDS}/bitDifferences{diff}/maxAbs{maximum:.12g}/relativeL2{relative:.12g}')
    print(f'STRUCTURE_VALID=1 NUMERICAL_BITEXACT={int(exact)} NO_TOLERANCE_USED;NOT native/physical/Work/TPS qualification')
    if args.diagnostic_only:
        print('DIAGNOSTIC_ONLY exit0 is NOT a numerical acceptance or hardware permission')
        return 0
    return 0 if exact else 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print(f'TRACE_REFUSAL {error}', file=sys.stderr)
        sys.exit(2)
