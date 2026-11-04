#!/usr/bin/env python3
"""Macro trial retains the complete qualified one-shot physical parent."""
import pathlib
import runpy
import subprocess

here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_vector_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_vector_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_macro_q4_rate_physical.sh'
text=path.read_text();subprocess.run(['sh','-n',str(path)],check=True)
changes={
 '# ONE full-card vector-Q4 tag92 rate trial;fixed generation16 admission.':'# ONE full-card macro-Q4 tag93 rate trial;fixed generation18 admission.',
 'pioneer-q38-full-vector-q4-20261104-a':'pioneer-q38-full-macro-q4-20261104-a',
 'tenstorrent_pci_probe-full-vector-q4-92':'tenstorrent_pci_probe-full-macro-q4-93',
 'HaikuAssistantQwen27FullOffload-vector-q4':'HaikuAssistantQwen27FullOffload-macro-q4',
 'candidate_hash=c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014':'candidate_hash=61a8cce47e5294f6f6263fe22a331859c22576f7e268416a3433f00c81e7d79e',
 'worker_hash=1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9':'worker_hash=103e0c734fbb1f63f2beea6b3de617e42a3699af7450b4ee895e8a056cd81160',
 'client_hash=9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af':'client_hash=46f4fcb520cf2c5ffa6767c3b3d17371ff5c1d22aeb5dcd98d15100262baa7b3',
 'native_tag=2026110400010092':'native_tag=2026110400010093',
 'stable_generation=16 native_generation=17 restored_generation=18':'stable_generation=18 native_generation=19 restored_generation=20'}
expected=old
for source,target in changes.items():
 assert expected.count(source)==1 and text.count(target)==1,source
 expected=expected.replace(source,target)
assert text==expected,'NO altered flow/admission/exchange/drain/tail/restore is permitted'
print('PASS macro-Q4 full-card93 parent:ENTIRE prior source exact except immutable revision/stage/hash/generation substitutions;'
      'all inherited fake failure cases FIRST;NO hardware/TPS grant',flush=True)
