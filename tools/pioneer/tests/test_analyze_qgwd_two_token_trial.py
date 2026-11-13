#!/usr/bin/env python3
"""Synthetic transcript tests; no model or device execution."""
from pathlib import Path
import subprocess
import sys
import tempfile

analyzer = Path(__file__).resolve().parent.parent / 'analyze_qgwd_two_token_trial.py'
log = '''HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet
HYBRID_PREFLIGHT 1351/1351
HYBRID_UPLOAD 21094223872/21094223872
HYBRID_START_REQUEST diagnostic only; no completion implied
HYBRID_STARTED upload_and_start_seconds=1.000
HYBRID_RESULT prompt_tokens=1 generated_tokens=2 inference_seconds=10.000 (includes START and prompt processing)
QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.200000
HYBRID_TOKEN 70253
HYBRID_TOKEN 70253
HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated
'''
with tempfile.TemporaryDirectory(prefix='qgwd-two-analysis-', dir='/private/tmp') as directory:
    trial = Path(directory)
    def reset():
        (trial / 'state').write_text('CANDIDATE_CLOSED_STABLE_RESTORED_QUALITY_UNVERIFIED\n')
        (trial / 'trial.exit').write_text('0\n')
        (trial / 'trial.stderr').write_bytes(b'')
        (trial / 'trial.stdout').write_text(log)
    def run():
        return subprocess.run([sys.executable, str(analyzer), str(trial)], capture_output=True, text=True)
    reset(); result = run()
    assert result.returncode == 0 and 'MATCH:' in result.stdout and not result.stderr
    assert 'CPU_REFERENCE_POSITION_ZERO 70253' in result.stdout
    assert 'SECOND_POSITION_NOT_COMPARABLE_CPU_INPUT_WAS_17' in result.stdout
    assert 'two-token sequence matches recorded card sequence' in result.stdout
    (trial / 'trial.stdout').write_text(log.replace('HYBRID_TOKEN 70253\nHYBRID_TOKEN 70253',
                                                    'HYBRID_TOKEN 70253\nHYBRID_TOKEN 42'))
    result = run()
    assert result.returncode == 1 and 'MISMATCH:' in result.stdout and not result.stderr
    assert 'second position is NOT a CPU comparison' in result.stdout
    reset()
    early_eos = (log.replace('generated_tokens=2', 'generated_tokens=1')
                 .replace('HYBRID_TOKEN 70253\nHYBRID_TOKEN 70253', 'HYBRID_TOKEN 248044'))
    (trial / 'trial.stdout').write_text(early_eos); result = run()
    assert result.returncode == 1 and 'MISMATCH:' in result.stdout and not result.stderr
    for name, contents in (('state', 'CANDIDATE_RUNNING_OWNERSHIP_UNKNOWN\n'),
                           ('trial.exit', '1\n'), ('trial.stderr', 'failure\n'),
                           ('trial.stdout', log.replace('HYBRID_COMPLETE', 'BROKEN_COMPLETE'))):
        reset(); (trial / name).write_text(contents); result = run()
        assert result.returncode == 2 and not result.stdout and result.stderr.startswith('ERROR:'), name
print('PASS synthetic position-zero CPU and historical card comparisons; NO DEVICE OR QUALITY CLAIM')
