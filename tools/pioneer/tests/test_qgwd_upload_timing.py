#!/usr/bin/env python3
"""Shape checks only; all timing and model output values are synthetic."""
import importlib.util
from pathlib import Path

path=Path(__file__).resolve().parent.parent/'check_qgwd_upload_timing.py'
spec=importlib.util.spec_from_file_location('upload_timing_checker',path)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
timing=('HYBRID_UPLOAD_TIMING valid=1 read_calls=100 read_bytes=21094223872 read_ns=1000000000 '
        'span_calls=21907 span_bytes=21094223872 span_ioctl_ns=2000000000;HOST_WALL_TIME_NOT_RAW_PCIE')
rows=['HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet','HYBRID_PREFLIGHT 1351/1351',
      'HYBRID_UPLOAD 21094223872/21094223872','HYBRID_START_REQUEST diagnostic only; no completion implied',timing,
      'HYBRID_STARTED upload_and_start_seconds=24.000',
      'HYBRID_RESULT prompt_tokens=1 generated_tokens=2 inference_seconds=10.000 (includes START and prompt processing)',
      'QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT 0.200000','HYBRID_TOKEN 42','HYBRID_TOKEN 43',
      'HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated']
assert 'SHAPE_CLEAN' in module.check('\n'.join(rows)+'\n')
cases=[rows[:4]+rows[5:],rows[:5]+[timing]+rows[5:],
       [r.replace('valid=1','valid=0') for r in rows],
       [r.replace('span_calls=21907','span_calls=21906') for r in rows],
       [r.replace('read_bytes=21094223872','read_bytes=1') for r in rows],
       [r.replace('read_ns=1000000000','read_ns=25000000000') for r in rows],
       rows[:4]+[rows[5],timing]+rows[6:],rows+['unexpected']]
for case in cases:
    try:module.check('\n'.join(case)+'\n')
    except ValueError:continue
    raise AssertionError(case)
print('PASS timing transcript count, clock-sum, ordering and underlying tag142 shape refusals; synthetic only')
