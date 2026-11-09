#!/usr/bin/env python3
"""Synthetic trial packaging around the real pinned CPU reference; NO DEVICE."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from compare_pioneer_q38_qgw8_hidden import CARD_TAG, WIDTH

if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute():
    raise SystemExit('usage: ABS_PINNED_REFERENCE_DIR')
reference = Path(sys.argv[1]); trace = reference/'original-token0.bin'; cpu = reference/'cpu-hidden.bin'
source = Path(__file__).with_name('analyze_pioneer_q38_qgw8_hidden_trial.py')
header = [0x31444851, 1, 0, 248319, 64, WIDTH, 1, 1,
          CARD_TAG & 0xffffffff, CARD_TAG >> 32, 5, 0, 2, 0, 0, 0]
cpu_data = cpu.read_bytes()
card = struct.pack('<16I', *header) + cpu_data[64+WIDTH*4:64+WIDTH*8] + struct.pack('<I', 0x454e4448)
digest = hashlib.sha256(card).hexdigest()
log = ('HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet\n'
       'HYBRID_PREFLIGHT 1351/1351\nHYBRID_UPLOAD 21167624192/21167624192\n'
       'HYBRID_START_REQUEST diagnostic only; no completion implied\n'
       'HYBRID_STARTED upload_and_start_seconds=1.000\n'
       'HYBRID_RESULT prompt_tokens=1 generated_tokens=1 inference_seconds=1.000 (includes START and prompt processing)\n'
       'QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 1.000000\n'
       'HYBRID_TOKEN 70253\nHYBRID_COMPLETE native FINISH and close succeeded; output quality not validated\n'
       f'QGW8_HIDDEN_SNAPSHOT sha256={digest} bytes=20548; DATA_ONLY_AFTER_CLOSE\n')
with tempfile.TemporaryDirectory(prefix='qgw8-trial-analysis-', dir='/private/tmp') as tmp:
    trial = Path(tmp)
    def reset():
        (trial/'state').write_text('CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED\n')
        (trial/'trial.exit').write_text('0\n'); (trial/'trial.stderr').write_bytes(b'')
        (trial/'trial.stdout').write_text(log); (trial/'hidden.bin').write_bytes(card)
    def run():
        return subprocess.run([sys.executable, str(source), str(trial), str(trace), str(cpu)], capture_output=True, text=True)
    reset(); result = run()
    assert result.returncode == 0 and 'reference=converted relative_l2=0 ' in result.stdout, (result.stdout, result.stderr)
    assert 'OFFLINE_ONLY_NOT_CARD_PROVENANCE_OR_QUALITY_OR_SPEED' in result.stdout
    cases = [('state', lambda: (trial/'state').write_text('CANDIDATE_RUNNING_OWNERSHIP_UNKNOWN\n')),
             ('exit', lambda: (trial/'trial.exit').write_text('1\n')),
             ('stderr', lambda: (trial/'trial.stderr').write_text('x')),
             ('transcript', lambda: (trial/'trial.stdout').write_text(log.replace('HYBRID_COMPLETE', 'BROKEN_COMPLETE'))),
             ('snapshot', lambda: (trial/'hidden.bin').write_bytes(card[:-1]+b'\0'))]
    for name, mutate in cases:
        reset(); mutate(); result = run()
        assert result.returncode != 0 and not result.stdout and result.stderr.startswith('ERROR:'), (name, result.stdout, result.stderr)
print('PASS synthetic completed-trial binding and five failure cases; NO DEVICE or card provenance')
