#!/usr/bin/env python3
"""Independent frozen81 CHILD-output parser;not parent/native closure authority."""
import hashlib
import pathlib
import re
import sys

if not __debug__:
    raise SystemExit('Assertions required')
REFERENCE = pathlib.Path(__file__).resolve().parents[1] / 'benchmarks/qwen38-native-model-band-preparation-20261104/learned-reference.log'
BANNERS = ('AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
           'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK')
FINAL = 'PASS_NATIVE_GDDR_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
SUFFIX = ' requests320 flits2880 saved_l152812 saved_gddr46080;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
def reference():
    raw = REFERENCE.read_bytes()
    if hashlib.sha256(raw).hexdigest() != '35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83':
        raise ValueError('Independent reference changed')
    expected = {}
    for line in raw.decode('ascii').splitlines():
        match = re.fullmatch(r'LEARNED_FRACTION_REFERENCE sample([012]) first([0-9]+) row([0-9]+) word([0-9a-f]{8})', line)
        if match:
            sample, first, row = map(int, match.group(1, 2, 3))
            if first != (0, 8704, 17376)[sample] or row >= 32 or (sample, row) in expected:
                raise ValueError('Independent reference geometry/duplicate')
            expected[sample, row] = match.group(4)
    if len(expected) != 96:
        raise ValueError('Independent reference incomplete')
    return expected
def verify(text):
    """Verify actual exact fixed81 child text ONLY,no driver/device authority."""
    if not text.endswith('\n') or len(text) > 262144 or '\r' in text or '\0' in text:
        raise ValueError('Child framing/size refused')
    lines = text.splitlines()
    if len(lines) != 198 or tuple(lines[:2]) != BANNERS or lines[-1] != FINAL:
        raise ValueError('Child banners/count/final refused')
    expected = reference()
    index = 2
    polls = []
    for sample, first in enumerate((0, 8704, 17376)):
        for job in range(2):
            for row in range(32):
                word = expected[sample, row]
                line = f'NATIVE_GDDR_BAND_RETURNED_WORD sample{sample} first{first} pass{job} row{row} got{word} expected{word}'
                if lines[index] != line:
                    raise ValueError(f'Child word/order/reference refused at sample{sample}/pass{job}/row{row}')
                index += 1
        prefix = f'PASS_NATIVE_GDDR_BAND sample{sample} tensor7 first{first} width5120 phases4 dual_jobs2 exact_words64 polls'
        match = re.fullmatch(re.escape(prefix) + r'([1-9][0-9]*) generation11' + re.escape(SUFFIX), lines[index])
        if not match or int(match[1]) > 20000:
            raise ValueError('Child poll/epoch/fidelity/counter/journal/restore marker refused')
        polls.append(int(match[1]));index += 1
    if index != len(lines) - 1:
        raise ValueError('Child ordering/duplicate refused')
    return {'samples': 3, 'words': 192, 'ends': 3, 'generation': 11, 'polls': polls}
def fixture():
    expected = reference();lines = list(BANNERS)
    for sample, first in enumerate((0, 8704, 17376)):
        for job in range(2):
            for row in range(32):
                word = expected[sample, row]
                lines.append(f'NATIVE_GDDR_BAND_RETURNED_WORD sample{sample} first{first} pass{job} row{row} got{word} expected{word}')
        lines.append(f'PASS_NATIVE_GDDR_BAND sample{sample} tensor7 first{first} width5120 phases4 dual_jobs2 exact_words64 polls2 generation11' + SUFFIX)
    return '\n'.join(lines + [FINAL]) + '\n'
def self_test():
    normal = fixture();lines = normal.splitlines();cases = [normal[:-1], normal + 'NOISE\n', normal.replace('\n', '\r\n')]
    assert verify(normal)['polls'] == [2, 2, 2]
    for i in range(len(lines)):
        cases.append('\n'.join(lines[:i] + lines[i+1:]) + '\n')
        cases.append('\n'.join(lines[:i] + [lines[i]] + lines[i:]) + '\n')
        if lines[i].startswith('NATIVE_GDDR_BAND_RETURNED_WORD'):
            bad = list(lines);old = re.search(r'got([0-9a-f]{8}) expected\1$', bad[i]);assert old
            word = '%08x' % (int(old[1], 16) ^ 1)
            bad[i] = bad[i][:old.start()] + 'got' + word + ' expected' + word
            cases.append('\n'.join(bad) + '\n')
    for sample in range(3):
        marker = 66 + sample * 65
        for old, new in (('generation11', 'generation10'), ('polls2', 'polls0'), ('polls2', 'polls20001'),
                         ('phases4', 'phases1'), ('width5120', 'width6144'), ('requests320', 'requests321'),
                         ('flits2880', 'flits2879'), ('saved_l152812', 'saved_l152811'),
                         ('saved_gddr46080', 'saved_gddr0'), ('CLOCK_PCI_RESTORED', 'PCI_RESTORED')):
            bad = list(lines);assert old in bad[marker];bad[marker] = bad[marker].replace(old, new)
            cases.append('\n'.join(bad) + '\n')
    old = pathlib.Path(__file__).resolve().parents[1] / 'benchmarks/qwen38-native-model-band-physical-20261104/learned.stdout'
    cases.append(old.read_text()) # old exact80 output is NOT new81 proof
    for text in cases:
        try:
            verify(text)
        except ValueError:
            continue
        raise AssertionError('Corrupted/old fixture accepted')
    print(f'PASS independent81 child parser:1 valid/{len(cases)} refused;strict Fraction words/order/epoch/counters/journals;old80 not promoted;NO parent/native/device/model/speed proof')
if __name__ == '__main__':
    if sys.argv[1:] == ['--self-test']:
        self_test()
    elif len(sys.argv) == 3 and sys.argv[1] == '--check-child-output':
        result = verify(pathlib.Path(sys.argv[2]).read_text())
        print('PASS exact frozen81 CHILD output ONLY:', result, ';parent/native/physical closure must be independently observed')
    else:
        raise SystemExit('Use --self-test or --check-child-output PATH;CPU read-only,NO GPU operation')
