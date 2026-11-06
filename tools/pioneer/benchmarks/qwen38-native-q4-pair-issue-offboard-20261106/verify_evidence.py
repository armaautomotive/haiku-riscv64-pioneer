#!/usr/bin/env python3
"""Saved OFFBOARD receipt consistency,not hardware/lifecycle authority."""
import hashlib
import json
from pathlib import Path

here=Path(__file__).resolve().parent
data=json.loads((here/'comparison.json').read_text())
assert data['source_counts']=={'main':937,'clean':934}
assert data['actual_children_each']==21 and data['actual_parent_exit_each']==data['all_child_stderr_bytes']==0
assert len(data['artifacts'])==32 and len(data['inactive_dirty_differences'])==6 and len(data['inactive_untracked_main_only'])==3
for name,record in data['archived_main_files'].items():
    body=(here/name).read_bytes()
    assert len(body)==record['bytes'] and hashlib.sha256(body).hexdigest()==record['sha256'],name
exits=json.loads((here/'actual-exits.json').read_text())
assert len(exits)==21 and set(exits.values())=={0}
assert data['artifacts']['native108/tenstorrent_pci_probe-native-q4-fpu-stream108']['sha256']=='d6d457fdb4a5535d84a82ae19b138d156ef295a0dcb66142274580a4bc78bf52'
assert data['artifacts']['client108/pioneer-native-compact-q4-pair-issue-diagnostic']['sha256']=='cc57303dda69059a7d9762eee56ded50e2c68d77035a25357921d8ccd9de3cad'
assert 'valid333 refused798' in (here/'native-endpoints.stdout').read_text()
assert 'clocks4 timerMutationAndReadFaults48' in (here/'timer-journal.stdout').read_text()
assert 'cases418 fullParentMissingRefusals173' in (here/'parent.stdout').read_text()
assert 'positives3 refusals36' in (here/'roles.stdout').read_text()
assert 'lease576 exact words over9 bands' in (here/'real-lease.stdout').read_text()
print('PASS SAVED OFFBOARD native108/21childrenMainClean/32matchingArtifacts/576exactWords/knownENDrestore_unknownRetains;NOT_HARDWARE_TIMING_OR_MODEL_TPS')
