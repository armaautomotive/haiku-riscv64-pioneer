#!/usr/bin/env python3
"""Independent fixed82 output gate;old81/fabricated partial records refuse."""
import pathlib
import sys
from pioneer_native_dram_load_output import BANNERS, PREFIX, SUFFIX, verify

assert __debug__, 'Assertions required'
assert len(sys.argv) == 1 or sys.argv[1:] == ['--alignment64-control']
aligned = len(sys.argv) == 2
active_banners = tuple(v.replace('LOAD_ONLY', 'LOAD64_ONLY') for v in BANNERS) if aligned else BANNERS
active_prefix = PREFIX.replace('LOAD_ONLY', 'LOAD64_ONLY').replace('saved_l152812', 'saved_l155372').replace('headers_checked2560', 'headers_checked5120') if aligned else PREFIX
def record(polls):
    return '\n'.join((*active_banners, active_prefix + str(polls) + SUFFIX, ''))
for polls in (1, 2, 20000):
    assert verify(record(polls), aligned) == polls
good = record(2)
bad = [None, b'', '', good[:-1], good + '\n', good.replace('\n', '\r\n'),
       good + 'PASS\n', '\n'.join(good.split('\n')[1:]),
       '\n'.join((active_banners[1], active_banners[0], active_prefix + '2' + SUFFIX, ''))]
for polls in (0, -1, 20001, 99999, 100000, '+2', '02', ' 2', '2 ', '2.0', '2e0'):
    bad.append(record(polls))
for old, new in (
        ('sample0', 'sample1'), ('tensor7', 'tensor8'), ('width5120', 'width6144'),
        ('requests320', 'requests319'), ('requests320', 'requests321'),
        ('flits2880', 'flits2879'), ('flits2880', 'flits2881'),
        ('saved_l155372' if aligned else 'saved_l152812', 'saved_l152811'), ('saved_gddr46080', 'saved_gddr46079'),
        ('payload_checked46080', 'payload_checked46079'),
        ('headers_checked5120' if aligned else 'headers_checked2560', 'headers_checked2559'), ('generation3', 'generation4'),
        ('EXPLICIT_END', 'NO_END'), ('RESTORED', 'UNKNOWN'),
        ('NO_FPU_MODEL_QUALITY_OR_SPEED_CLAIM', 'FULL_MODEL_PASS'),
        ('LOAD64_ONLY_NO_FPU_MODEL_OR_WORK' if aligned else 'LOAD_ONLY_NO_FPU_MODEL_OR_WORK', 'MODEL_WORK'), ('c600de03', 'c600de02')):
    assert old in good
    bad.append(good.replace(old, new))
for word in good.split('\n')[2].split(' '):
    bad.append(good.replace(word, word + 'X', 1))
    bad.append(good.replace(word, '', 1))
old = pathlib.Path(__file__).resolve().parents[1] / 'benchmarks/qwen38-native-dram-band-failed-20261104/gddr.stdout'
bad.append(old.read_text())
if aligned:
    bad += ['\n'.join((*BANNERS, PREFIX + '2' + SUFFIX, '')),
            (old.parent.parent / 'qwen38-native-dram-load-failed-20261104/load.stdout').read_text()]
for value in bad:
    try:
        verify(value, aligned)
    except ValueError:
        pass
    else:
        raise AssertionError('Partial/wrong loader record promoted')
print('PASS independent LOAD-ONLY%d child parser:3 valid/%d refused;old failed81 NOT accepted;NO native-byte/FPU/full-model/speed proof' % (83 if aligned else 82, len(bad)))
