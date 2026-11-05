#!/usr/bin/env python3
"""Fixed-config trial preserves WHOLE prior lifetime;NO hardware execution."""
import json,os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_queued_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_queued_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_fixed_config_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes=json.loads("[[\"ONE full-card queued-Q4 tag102 rate trial;fixed generation32 admission\",\"ONE full-card fixed-config-Q4 tag103 rate trial;fixed generation34 admission\"],[\"/boot/home/pioneer-q38-full-queued-q4-20261105-a\",\"/boot/home/pioneer-q38-full-fixed-config-q4-20261105-a\"],[\"tenstorrent_pci_probe-full-queued-q4-102\",\"tenstorrent_pci_probe-full-fixed-config-q4-103\"],[\"HaikuAssistantQwen27FullOffload-queued-q4\",\"HaikuAssistantQwen27FullOffload-fixed-config-q4\"],[\"8574549d046c51b369ec6298002017fff0b2de82d4968fed5e991eddd04be689\",\"608ae0a091b944c3e9ae96d08bdc4aa74a3bd059c56ee65a414445f29b1ced20\"],[\"549b34032aaaa29c41a7696052129dbf66743c52680f5e06fc1ae6b663ba54fe\",\"c50acf30e31890f0abf65bd061f2cdee65e3ec31aa30d43dcfabcfad9d870bff\"],[\"c3b0224228592314c0f5e2948bf3bdcd1a23ae6b8f58945b9397ec46ee7d8e26\",\"3988d149f607deca9c41c83b538b161a5c448af0b37a3d6253c5f5e62d1759db\"],[\"native_tag=2026110500010102\",\"native_tag=2026110500010103\"],[\"stable_generation=32 native_generation=33 restored_generation=34\",\"stable_generation=34 native_generation=35 restored_generation=36\"]]")
for source,target in changes:
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No admission,exit,unknown-failure,cleanup or restoration policy change permitted'
print('PASS fixed-config-Q4 tag103 parent WHOLE tested lifetime except9 exact revision/stage/generation substitutions;NO device or speed result',flush=True)
