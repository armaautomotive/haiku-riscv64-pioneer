#!/usr/bin/env python3
"""Test transcript comparator using invented BFP4 timing, never a device."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

if len(sys.argv) != 5:
    raise SystemExit('use BFP4_BAND Q4_INPUT Q4_EXPECTED Q4_PHYSICAL_LOG')
band, inputs, expected, qlog = map(Path, sys.argv[1:])
data = band.read_bytes()
if len(data) != 102544:
    raise ValueError('bad fixture')
values = struct.unpack_from('<32I', data, 102416)
lines = []
for turn in range(2):
    for row, word in enumerate(values):
        lines.append(f'NATIVE_GDDR_BAND_RETURNED_WORD sample0 first0 pass{turn} row{row} got{word:08x} expected{word:08x}')
for turn in range(2):
    lines.append(f'BFP4_GDDR_BAND_CYCLES sample0 pass{turn} cycles12345;INCLUDES_GDDR_AND_COMPUTE_NOT_MODEL_SPEED')
lines += ['P100A_RELOAD driver55 tag2026102100010012 generation20 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY',
          'MATCHED_BFP4_GDDR_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM']
verifier = Path(__file__).with_name('compare_pioneer_q38_matched_card_logs.py')
with tempfile.TemporaryDirectory(prefix='pioneer-matched-card-comparator-') as folder:
    log = Path(folder) / 'invented.log'
    def check(content, success):
        log.write_text('\n'.join(content)+'\n')
        result = subprocess.run([sys.executable,str(verifier),str(band),str(inputs),str(expected),str(qlog),str(log)],
                                capture_output=True,text=True)
        if (result.returncode == 0) != success:
            raise RuntimeError('unexpected comparator result: '+result.stderr)
        if success and 'TRANSCRIPTS_ONLY_ACTUAL_EXIT_AND_POSTFLIGHT_REQUIRED_NO_MODEL_TPS' not in result.stdout:
            raise RuntimeError('missing limitation')
    check(lines, True)
    check(lines[1:], False)
    check(lines+[lines[0]], False)
    changed=list(lines);changed[0]=changed[0].replace('got','gotf',1);check(changed,False)
    changed=list(lines);changed[-3]=changed[-3].replace('cycles12345','cycles0');check(changed,False)
    check(lines[:-1],False)
    changed=list(lines);changed[-2]=changed[-2].replace('generation20','generation19');check(changed,False)
print('PASS matched card transcript comparator: invented timing and six refusals; NO DEVICE')
