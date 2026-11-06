#!/usr/bin/env python3
"""Exact new109 subject over the complete already-tested parent policy."""
import pathlib
import runpy
import subprocess

here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_bf16_profile_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_bf16_profile_full_offload_physical.sh').read_text()
path=here.parent/'pioneer_q38_bf16_pair_issue_full_offload_physical.sh'
new=path.read_text();subprocess.run(['sh','-n',str(path)],check=True)
changes=[
["ONE full-card BF16-profile tag106 rate trial;fixed generation40 admission","ONE full-card paired-BF16 tag109 rate trial;fixed generation46 admission"],
["/boot/home/pioneer-q38-bf16-profile-106-a","/boot/home/pioneer-q38-bf16-pair-issue-109-a"],
["tenstorrent_pci_probe-bf16-profile","tenstorrent_pci_probe-bf16-pair-issue"],
["HaikuAssistantQwen27Bf16ProfileFullOffload","HaikuAssistantQwen27Bf16PairIssueFullOffload"],
["client=$stage/pioneer-q38-bf16-profile-rate-smoke","client=$stage/pioneer-q38-bf16-pair-issue-rate-smoke"],
["b635aab47e10b3bf657b290e27eb2a41460ba7db71866a0f9ecf475d70a0c954","adbb988ced6b3ccfdd4e73f43d4b9eac3101c5f0ca6289bb60654af9498223e1"],
["e3c784766d6ff46513157bc9ae5849e5aac73a02f4b9c77e48f7dbfb24f72a26","d6e10432f86dcecf2034986f887c6c2f5ab0e1314a2e3e2e5ed342cb6e5dce33"],
["509856b35bb9f8c7da34c5537f1a459bcd21ca7c95343a680fbb647dcaaa35c7","afe31e8d68cb0ccd5dc4ee74a11ad1c1bbcd0af3cd300cb6ce903e0936f6fe7b"],
["native_tag=2026110500010106","native_tag=2026110600010109"],
["stable_generation=40 native_generation=41 restored_generation=42","stable_generation=46 native_generation=47 restored_generation=48"],
]
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1
 old=old.replace(source,target)
assert old==new,'No admission,ownership,exit,UNKNOWN or restoration policy change'
print('PASS paired109 parent WHOLE tested lifetime except10 exact stage/tag/hash/generation substitutions;FAKE_ONLY_NOT_HARDWARE_OR_TPS')
