#!/usr/bin/env python3
"""Check resident cleanup sequence against the real one-shot backend."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[3] / 'src/add-ons/kernel/drivers/misc'
standard = (root / 'PioneerQ38TokenNativeBackend.inc').read_text()
resident = (root / 'PioneerQ38QgwdResidentClose.inc').read_text()
pattern = r'\b{}\((\d+),\s*([A-Za-z0-9_]+),\s*([A-Za-z0-9_]+)\)'
normal = re.findall(pattern.format('PIONEER_Q38_BACKEND_CLOSE_STAGE'), standard)
retained = re.findall(pattern.format('PIONEER_QGWD_RESIDENT_CLOSE'), resident)
assert len(normal) == 10 and normal == retained
assert [int(step) for step, _, _ in retained] == list(range(1, 11))
assert 'scope.Finish(result)' in standard
assert 'scope.Finish' not in resident
assert 'RetainQ38DispatchOwner(*c,retained)' in resident
print('PASS resident close uses the same ten real cleanup stages and retains the reservation; SOURCE PARITY ONLY')
