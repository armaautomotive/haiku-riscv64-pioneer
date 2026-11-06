#!/usr/bin/env python3
"""Distinct110 subject; preserve the entire tested one-shot parent policy."""
import os
import pathlib
import runpy
import subprocess

if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Checks required')
here = pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here / 'pioneer_q38_bf16_pair_issue_parent_test.py'), run_name='__main__')
old = (here.parent / 'pioneer_q38_bf16_pair_issue_full_offload_physical.sh').read_text()
path = here.parent / 'pioneer_q38_bf16_deferred_full_offload_physical.sh'
new = path.read_text()
subprocess.run(['sh', '-n', str(path)], check=True)
changes = [
    ('ONE full-card paired-BF16 tag109 rate trial;fixed generation46 admission',
     'ONE full-card deferred-BF16 tag110 rate trial;fixed generation48 admission'),
    ('/boot/home/pioneer-q38-bf16-pair-issue-109-a', '/boot/home/pioneer-q38-bf16-deferred-110-a'),
    ('tenstorrent_pci_probe-bf16-pair-issue', 'tenstorrent_pci_probe-bf16-deferred'),
    ('HaikuAssistantQwen27Bf16PairIssueFullOffload', 'HaikuAssistantQwen27Bf16DeferredFullOffload'),
    ('client=$stage/pioneer-q38-bf16-pair-issue-rate-smoke', 'client=$stage/pioneer-q38-bf16-deferred-rate-smoke'),
    ('adbb988ced6b3ccfdd4e73f43d4b9eac3101c5f0ca6289bb60654af9498223e1',
     'fab6d0e741cbb06ab540af07d4d1d4864cf84ec29eaf02b5048d11464bc771e9'),
    ('d6e10432f86dcecf2034986f887c6c2f5ab0e1314a2e3e2e5ed342cb6e5dce33',
     '4b526caefad509819b6453ebc99e0199e7e5e6ad079534f76e848cfe66706d00'),
    ('afe31e8d68cb0ccd5dc4ee74a11ad1c1bbcd0af3cd300cb6ce903e0936f6fe7b',
     '47eb2c07d49e540db8c6bd02a38c6cad9dac8147ddeb60e83d17b89c75a33dbd'),
    ('native_tag=2026110600010109', 'native_tag=2026110600010110'),
    ('stable_generation=46 native_generation=47 restored_generation=48',
     'stable_generation=48 native_generation=49 restored_generation=50'),
]
for source, target in changes:
    assert old.count(source) == 1 and new.count(target) == 1
    old = old.replace(source, target)
assert old == new, 'No admission,ownership,exit,UNKNOWN or restoration policy change'
print('PASS deferred110 parent WHOLE tested lifetime except10 exact stage/tag/hash/generation substitutions;FAKE_ONLY_NOT_HARDWARE_OR_TPS')
