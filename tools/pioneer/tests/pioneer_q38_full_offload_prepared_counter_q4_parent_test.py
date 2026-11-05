#!/usr/bin/env python3
"""Same guarded physical lifetime;only paired revision identifiers differ."""
import os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_prepared_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_prepared_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_prepared_counter_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=[
 [
  "ONE full-card prepared-Q4 tag97 rate trial",
  "ONE full-card prepared-counter-Q4 tag98 rate trial"
 ],
 [
  "/boot/home/pioneer-q38-full-prepared-q4-20261105-a",
  "/boot/home/pioneer-q38-full-prepared-counter-q4-20261105-a"
 ],
 [
  "tenstorrent_pci_probe-full-prepared-q4-97",
  "tenstorrent_pci_probe-full-prepared-counter-q4-98"
 ],
 [
  "HaikuAssistantQwen27FullOffload-prepared-q4",
  "HaikuAssistantQwen27FullOffload-prepared-counter-q4"
 ],
 [
  "f4d98a7f6c561703ec0ed9111ddaa7f263898c02e305804c2af918347643ed88",
  "edd270242413f6fdc7b6310aaedaa0daa89d1b1250c8c0533959ca356bc39be1"
 ],
 [
  "10df2a16c8fee1208743117418a5b155e986a6868c09ea237c6e0dac56a37bd0",
  "37bf461710e8890581e525a747607c0792c1a555fdb82eca1a5ce75b6db0aa85"
 ],
 [
  "588133ca6b05dadd1bc6b3075e08542813116bec73ec7dce0a5f23dd5afe6c0f",
  "ec343e34dc3257572487bf66d5b8481896a4c7d91dcc8ae5ae3291610ac0e03e"
 ],
 [
  "native_tag=2026110500010097",
  "native_tag=2026110500010098"
 ]
]
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No other admission/exchange/actual-exit/restoration alteration permitted'
print('PASS prepared counter Q4 tag98 parent exact tested lifetime except8 paired revision substitutions;NO card or speed claim',flush=True)
