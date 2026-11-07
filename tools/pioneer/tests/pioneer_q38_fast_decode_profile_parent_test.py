#!/usr/bin/env python3
"""Exact prior tested physical flow, new identity only. No hardware IO."""
from pathlib import Path
import runpy
import subprocess

here = Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_rate_parent_test.py'), run_name='__main__')
original = (here.parent/'pioneer_q38_full_offload_vector_q4_rate_physical.sh').read_text()
path = here.parent/'pioneer_q38_fast_decode_profile_physical.sh'
text = path.read_text()
subprocess.run(['sh', '-n', str(path)], check=True)
changes = {
 '# ONE full-card vector-Q4 tag92 rate trial;fixed generation16 admission.': '# ONE fast-engine generation-profile tag116 trial;fixed generation8 admission.',
 'pioneer-q38-full-vector-q4-20261104-a': 'pioneer-q38-fast-decode-116-a',
 'tenstorrent_pci_probe-full-vector-q4-92': 'tenstorrent_pci_probe-fast-decode116',
 'HaikuAssistantQwen27FullOffload-vector-q4': 'HaikuAssistantQwen27FastDecodeFullOffload',
 'client=$stage/pioneer-q38-full-offload-rate-smoke': 'client=$stage/pioneer-q38-fast-decode-rate-smoke',
 'candidate_hash=c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014': 'candidate_hash=f592d38d635d017b0e36f60ba58e3de6da5912aadd01a99f6dd171abfc9c7816',
 'worker_hash=1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9': 'worker_hash=55ef6bc39f21e0291746af322ee2f55f8a242f2cc76a52169dd22f3eb706cc35',
 'client_hash=9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af': 'client_hash=72be2cde10f30fda938e1c49f55033fbab50018d4679e2d0e776ed787e4bad60',
 'native_tag=2026110400010092': 'native_tag=2026110600010116',
 'stable_generation=16 native_generation=17 restored_generation=18': 'stable_generation=8 native_generation=9 restored_generation=10',
}
for source, target in changes.items():
    assert original.count(source) == 1 and text.count(target) == 1, source
    original = original.replace(source, target)
assert text == original, 'no additional control-flow/admission/exchange/cleanup changes permitted'
print('PASS fast generation116 parent: entire tested vector92 flow exact except immutable stage/hash/tag/generation identity; prior fake failure cases FIRST;NO_DEVICE_OR_SPEED', flush=True)
