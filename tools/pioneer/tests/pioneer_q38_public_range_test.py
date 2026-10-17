"""Pure bounded-source completion policy tests; no sockets/files/models."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from pioneer_q38_public_range import all_sent, may_send

allowed = {"leaf0", "leaf3", "leaf4"}
assert not all_sent({}, allowed, 1)
assert not all_sent({}, set(), 1)
completed = {"leaf0": 1}
assert may_send(completed, "leaf0", 1)  # preserved fresh-repair retry
completed["leaf0"] = 2
completed["leaf3"] = 1
assert not all_sent(completed, allowed, 1)
completed["leaf4"] = 1
assert all_sent(completed, allowed, 1)  # duplicates cannot prevent closure

completed = {}
for round_index in range(3):
    for key in sorted(allowed):
        assert may_send(completed, key, 3)
        assert not all_sent(completed, allowed, 3)
        completed[key] = completed.get(key, 0) + 1
assert all_sent(completed, allowed, 3)
assert all(not may_send(completed, key, 3) for key in allowed)
print("PASS: source closes after3 distinct repairs despite a resend,or EXACTLY9 fast-test sends;no sockets/files/model/device work")
