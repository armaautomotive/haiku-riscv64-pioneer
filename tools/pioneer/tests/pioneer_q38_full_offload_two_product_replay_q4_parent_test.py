#!/usr/bin/env python3
"""Replay13 trial preserves the entire previously qualified physical lifetime."""
import os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_prepared_profile_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_prepared_profile_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_two_product_replay_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=[
 [
  "ONE full-card prepared-profile-Q4 tag99 rate trial;fixed generation26 admission",
  "ONE full-card replay13-two-product-Q4 tag100 rate trial;fixed generation28 admission"
 ],
 [
  "/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a",
  "/boot/home/pioneer-q38-full-two-product-replay-q4-20261105-a"
 ],
 [
  "tenstorrent_pci_probe-full-prepared-profile-q4-99",
  "tenstorrent_pci_probe-full-two-product-replay-q4-100"
 ],
 [
  "HaikuAssistantQwen27FullOffload-prepared-profile-q4",
  "HaikuAssistantQwen27FullOffload-two-product-replay-q4"
 ],
 [
  "50170113b3b55d973b104c82afbad7987c1dd7b71e1935e930eea6ebb4eb7e45",
  "ea526a252a4793d8f20fba8e4cedc47f4726e6426f29ef5c58791a0086181949"
 ],
 [
  "f47b04f4f0e21211520d0d8a2285353c0a40807fc0f016daa7f3e2234870ef8d",
  "088ab8eb999a81221d38d8b4aa4bdea33198c46f726efb7765d8b0e4700d2265"
 ],
 [
  "f810d61b543f07751ad1a3667f250afe3f4dc72454c8b6e9acbb1e7f60c0a5e0",
  "2ad68fab59193abeef362e00e3719abfcf992891d300edc8f708b942bd01c98b"
 ],
 [
  "native_tag=2026110500010099",
  "native_tag=2026110500010100"
 ],
 [
  "stable_generation=26 native_generation=27 restored_generation=28",
  "stable_generation=28 native_generation=29 restored_generation=30"
 ]
]
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,exit,unknown-failure,cleanup or restoration policy change permitted'
print('PASS replay13 tag100 parent WHOLE tested lifetime except9 exact revision/stage/generation substitutions;NO device or speed result',flush=True)
