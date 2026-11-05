#!/usr/bin/env python3
"""Queued trial preserves the WHOLE previously qualified physical lifetime."""
import json,os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_instruction_prefetch_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_instruction_prefetch_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_queued_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=json.loads("[[\"ONE full-card instruction-prefetch-Q4 tag101 rate trial;fixed generation30 admission\",\"ONE full-card queued-Q4 tag102 rate trial;fixed generation32 admission\"],[\"/boot/home/pioneer-q38-full-instruction-prefetch-q4-20261105-a\",\"/boot/home/pioneer-q38-full-queued-q4-20261105-a\"],[\"tenstorrent_pci_probe-full-instruction-prefetch-q4-101\",\"tenstorrent_pci_probe-full-queued-q4-102\"],[\"HaikuAssistantQwen27FullOffload-instruction-prefetch-q4\",\"HaikuAssistantQwen27FullOffload-queued-q4\"],[\"fbca121f3394491e702f66213f6d5e9d3e3fc38a749b07a9821e50a9543c64eb\",\"8574549d046c51b369ec6298002017fff0b2de82d4968fed5e991eddd04be689\"],[\"f0462866387e5a4ab422a43c5b0184724091186a233ee59cc6e7e2b31ea9ac18\",\"549b34032aaaa29c41a7696052129dbf66743c52680f5e06fc1ae6b663ba54fe\"],[\"55894b0cf48f74409c6648cd64ca98851552dceebfbff3a02e6be5a02a0460cd\",\"c3b0224228592314c0f5e2948bf3bdcd1a23ae6b8f58945b9397ec46ee7d8e26\"],[\"native_tag=2026110500010101\",\"native_tag=2026110500010102\"],[\"stable_generation=30 native_generation=31 restored_generation=32\",\"stable_generation=32 native_generation=33 restored_generation=34\"]]")
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,exit,unknown-failure,cleanup or restoration policy change permitted'
print('PASS queued-Q4 tag102 parent WHOLE tested lifetime except9 exact revision/stage/generation substitutions;NO device or speed result',flush=True)
