#!/usr/bin/env python3
"""Same full physical lifetime;only exact timing revision/stage/generation changes."""
import os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_prepared_counter_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_prepared_counter_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_prepared_profile_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=[
 [
  "ONE full-card prepared-counter-Q4 tag98 rate trial;fixed generation24 admission",
  "ONE full-card prepared-profile-Q4 tag99 rate trial;fixed generation26 admission"
 ],
 [
  "/boot/home/pioneer-q38-full-prepared-counter-q4-20261105-a",
  "/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a"
 ],
 [
  "tenstorrent_pci_probe-full-prepared-counter-q4-98",
  "tenstorrent_pci_probe-full-prepared-profile-q4-99"
 ],
 [
  "HaikuAssistantQwen27FullOffload-prepared-counter-q4",
  "HaikuAssistantQwen27FullOffload-prepared-profile-q4"
 ],
 [
  "edd270242413f6fdc7b6310aaedaa0daa89d1b1250c8c0533959ca356bc39be1",
  "50170113b3b55d973b104c82afbad7987c1dd7b71e1935e930eea6ebb4eb7e45"
 ],
 [
  "37bf461710e8890581e525a747607c0792c1a555fdb82eca1a5ce75b6db0aa85",
  "f47b04f4f0e21211520d0d8a2285353c0a40807fc0f016daa7f3e2234870ef8d"
 ],
 [
  "ec343e34dc3257572487bf66d5b8481896a4c7d91dcc8ae5ae3291610ac0e03e",
  "f810d61b543f07751ad1a3667f250afe3f4dc72454c8b6e9acbb1e7f60c0a5e0"
 ],
 [
  "native_tag=2026110500010098",
  "native_tag=2026110500010099"
 ],
 [
  "stable_generation=24 native_generation=25 restored_generation=26",
  "stable_generation=26 native_generation=27 restored_generation=28"
 ]
]
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,exit,unknown-failure,cleanup or restoration policy change permitted'
print('PASS profile tag99 physical parent WHOLE tested lifetime except9 exact revision/stage/generation substitutions;NO device or speed claim',flush=True)
