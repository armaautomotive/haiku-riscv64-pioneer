#!/usr/bin/env python3
"""New physical parent must retain the ENTIRE tested rate flow/functions."""
import pathlib
import runpy
import subprocess

here=pathlib.Path(__file__).resolve().parent
# Actual original parent failure injection tests,not merely a tail match.
runpy.run_path(str(here/'pioneer_q38_full_offload_rate_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_vector_q4_rate_physical.sh'
text=path.read_text();subprocess.run(['sh','-n',str(path)],check=True)
changes={
 '# ONE full-card token-rate tag90 trial;fixed generation14 admission.':'# ONE full-card vector-Q4 tag92 rate trial;fixed generation16 admission.',
 'pioneer-q38-full-metrics-20261104-a':'pioneer-q38-full-vector-q4-20261104-a',
 'tenstorrent_pci_probe-full-metrics90':'tenstorrent_pci_probe-full-vector-q4-92',
 'HaikuAssistantQwen27FullOffload-metrics':'HaikuAssistantQwen27FullOffload-vector-q4',
 'candidate_hash=2551b50a52f6a2f7c98ca391fa1742640d5d777f23379663733679980bd643d7':'candidate_hash=c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014',
 'worker_hash=989f54c394a42c43b2bf9e0fd64844bfb378ec79fb8fbe65643e8f9bad03242a':'worker_hash=1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9',
 'client_hash=d50896e160e4e7015ce0942ab5d83a0293a356fc90ddb0980be4313f38cfec4e':'client_hash=9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af',
 'native_tag=2026110400010090':'native_tag=2026110400010092',
 'stable_generation=14 native_generation=15 restored_generation=16':'stable_generation=16 native_generation=17 restored_generation=18'}
expected=old
for source,target in changes.items():
    assert expected.count(source)==1 and text.count(target)==1,source
    expected=expected.replace(source,target)
assert text==expected,'NO other control-flow/admission/exchange/final-tail/restore alteration permitted'
print('PASS vector-Q4 full-card92 rate parent:ENTIRE prior source EXACT except9 immutable revision/stage/hash/generation substitutions;ALL prior fake failure cases FIRST;NO hardware or speed result',flush=True)
