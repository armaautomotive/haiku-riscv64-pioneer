#!/usr/bin/env python3
"""Entire tested rate-parent policy; only exact new subject identities vary."""
from pathlib import Path
import runpy
import subprocess

assert __debug__
here=Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_prepared_profile_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_prepared_profile_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_bf16_profile_full_offload_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=[
 ('ONE full-card prepared-profile-Q4 tag99 rate trial;fixed generation26 admission','ONE full-card BF16-profile tag106 rate trial;fixed generation40 admission'),
 ('/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a','/boot/home/pioneer-q38-bf16-profile-106-a'),
 ('tenstorrent_pci_probe-full-prepared-profile-q4-99','tenstorrent_pci_probe-bf16-profile'),
 ('HaikuAssistantQwen27FullOffload-prepared-profile-q4','HaikuAssistantQwen27Bf16ProfileFullOffload'),
 ('client=$stage/pioneer-q38-full-offload-rate-smoke','client=$stage/pioneer-q38-bf16-profile-rate-smoke'),
 ('50170113b3b55d973b104c82afbad7987c1dd7b71e1935e930eea6ebb4eb7e45','b635aab47e10b3bf657b290e27eb2a41460ba7db71866a0f9ecf475d70a0c954'),
 ('f47b04f4f0e21211520d0d8a2285353c0a40807fc0f016daa7f3e2234870ef8d','e3c784766d6ff46513157bc9ae5849e5aac73a02f4b9c77e48f7dbfb24f72a26'),
 ('f810d61b543f07751ad1a3667f250afe3f4dc72454c8b6e9acbb1e7f60c0a5e0','509856b35bb9f8c7da34c5537f1a459bcd21ca7c95343a680fbb647dcaaa35c7'),
 ('native_tag=2026110500010099','native_tag=2026110500010106'),
 ('stable_generation=26 native_generation=27 restored_generation=28','stable_generation=40 native_generation=41 restored_generation=42'),
]
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,lifetime,exit,failure or restoration policy change permitted'
print('PASS BF16 profile106 parent WHOLE tested rate lifetime except10 exact subject substitutions;FAKE endpoints only,NOT hardware admission or speed')
