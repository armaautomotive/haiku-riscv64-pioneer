#!/usr/bin/env python3
"""Prefetch trial preserves the WHOLE previously qualified physical lifetime."""
import json,os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_two_product_replay_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_two_product_replay_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_instruction_prefetch_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=json.loads("[[\"ONE full-card replay13-two-product-Q4 tag100 rate trial;fixed generation28 admission\",\"ONE full-card instruction-prefetch-Q4 tag101 rate trial;fixed generation30 admission\"],[\"/boot/home/pioneer-q38-full-two-product-replay-q4-20261105-a\",\"/boot/home/pioneer-q38-full-instruction-prefetch-q4-20261105-a\"],[\"tenstorrent_pci_probe-full-two-product-replay-q4-100\",\"tenstorrent_pci_probe-full-instruction-prefetch-q4-101\"],[\"HaikuAssistantQwen27FullOffload-two-product-replay-q4\",\"HaikuAssistantQwen27FullOffload-instruction-prefetch-q4\"],[\"ea526a252a4793d8f20fba8e4cedc47f4726e6426f29ef5c58791a0086181949\",\"fbca121f3394491e702f66213f6d5e9d3e3fc38a749b07a9821e50a9543c64eb\"],[\"088ab8eb999a81221d38d8b4aa4bdea33198c46f726efb7765d8b0e4700d2265\",\"f0462866387e5a4ab422a43c5b0184724091186a233ee59cc6e7e2b31ea9ac18\"],[\"2ad68fab59193abeef362e00e3719abfcf992891d300edc8f708b942bd01c98b\",\"55894b0cf48f74409c6648cd64ca98851552dceebfbff3a02e6be5a02a0460cd\"],[\"native_tag=2026110500010100\",\"native_tag=2026110500010101\"],[\"stable_generation=28 native_generation=29 restored_generation=30\",\"stable_generation=30 native_generation=31 restored_generation=32\"]]")
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,exit,unknown-failure,cleanup or restoration policy change permitted'
print('PASS instruction-prefetch tag101 parent WHOLE tested lifetime except9 exact revision/stage/generation substitutions;NO device or speed result',flush=True)
