#!/usr/bin/env python3
"""Test the strict physical transcript gate using invented values, never a device."""
import pathlib
import subprocess

gate = pathlib.Path(__file__).resolve().parent.parent / 'pioneer_native_bfp4_dram_output.awk'
lines = ['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
         'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK']
for sample, first in enumerate((0, 8704, 17376)):
    for word in range(64):
        value = '%08x' % (word % 32)
        lines.append(f'NATIVE_GDDR_BAND_RETURNED_WORD sample{sample} first{first} pass{word//32} row{word%32} got{value} expected{value}')
    for turn in range(2):
        lines.append(f'BFP4_GDDR_BAND_CYCLES sample{sample} pass{turn} cycles12345;INCLUDES_GDDR_AND_COMPUTE_NOT_MODEL_SPEED')
    lines.append(f'PASS_NATIVE_BFP4_GDDR_BAND sample{sample} tensor7 first{first} width5120 phases2 dual_jobs2 exact_words64 polls1 generation17 requests640 flits3200 saved_l134892 saved_gddr25600;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
lines.append('PASS_NATIVE_BFP4_GDDR_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')

def run(items, generation='17'):
    return subprocess.run(['awk', '-v', 'generation=' + generation, '-f', str(gate)],
                          input='\n'.join(items)+'\n', text=True, capture_output=True).returncode

assert run(lines) == 0
cases = []
for i in range(len(lines)):
    cases.append(lines[:i] + lines[i+1:])
    cases.append(lines[:i] + [lines[i]] + lines[i:])
for before, after in [('got00000000', 'got00000001'), ('row0', 'row1'), ('first8704', 'first0'),
                      ('pass0', 'pass1'), ('cycles12345;', 'cycles0;'), ('cycles12345;', 'cyclesx;'),
                      ('generation17', 'generation18'), ('requests640', 'requests639'),
                      ('flits3200', 'flits3201'), ('saved_gddr25600', 'saved_gddr0'),
                      ('polls1', 'polls20001'), ('clean_ends3', 'clean_ends2')]:
    altered = list(lines)
    index = next(i for i, line in enumerate(altered) if before in line)
    altered[index] = altered[index].replace(before, after, 1)
    cases.append(altered)
cases.append(lines + ['unexpected output'])
assert all(run(case) != 0 for case in cases)
assert run(lines, '18') != 0 and run(lines, '0') != 0 and run(lines, '') != 0
print(f'PASS BFP4 transcript gate: exact valid output; {len(cases)+3} deletion, duplication, timing, data, epoch and closure refusals; NO DEVICE')
