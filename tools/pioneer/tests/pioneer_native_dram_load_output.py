#!/usr/bin/env python3
"""Parse explicitly selected fixed82/83 records;not full-model/math proof."""
import re

BANNERS = (
    'AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
    'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 sample0;LOAD_ONLY_NO_FPU_MODEL_OR_WORK',
)
PREFIX = ('PASS_NATIVE_DRAM_LOAD_ONLY sample0 tensor7 width5120 requests320 flits2880 '
          'saved_l152812 saved_gddr46080 payload_checked46080 headers_checked2560 polls')
SUFFIX = (' generation3;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;'
          'NO_FPU_MODEL_QUALITY_OR_SPEED_CLAIM')

def verify(contents, alignment64=False):
    """Return the actual bounded poll count only for the complete canonical record."""
    if not isinstance(contents, str) or type(alignment64) is not bool:
        raise ValueError('Exact text child record required')
    banners = BANNERS
    prefix = PREFIX
    if alignment64:
        # Explicit caller-selected role,NEVER auto-detect/accept an old record.
        banners = tuple(v.replace('LOAD_ONLY', 'LOAD64_ONLY') for v in BANNERS)
        prefix = PREFIX.replace('LOAD_ONLY', 'LOAD64_ONLY').replace('saved_l152812', 'saved_l155372').replace('headers_checked2560', 'headers_checked5120')
    lines = contents.split('\n')
    if len(lines) != 4 or tuple(lines[:2]) != banners or lines[3] != '':
        raise ValueError('Missing/reordered/extra source or result lines')
    match = re.fullmatch(re.escape(prefix) + r'([1-9][0-9]{0,4})' + re.escape(SUFFIX), lines[2])
    if not match or int(match[1]) > 20000:
        raise ValueError('Wrong loader geometry,checks,epoch,closure or polls')
    return int(match[1])
