#!/usr/bin/env python3
"""Generate the exact prior guarded flow with a distinct pinned identity.

Generation itself is read-only except for the fresh local output file; it
does not stage, install, open or run any GPU or remote device.
"""
import hashlib
from pathlib import Path
import subprocess
import sys

assert __debug__ and len(sys.argv)==3
build,output=map(Path,sys.argv[1:]);assert build.is_absolute() and output.is_absolute()
assert not output.exists() and not output.is_symlink()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
artifacts={
 'tenstorrent_pci_probe-snapshot117':'68803b259e93f0abcda40df71d216b200a7f0f8bda35589cf7c66d0835fdac2c',
 'HaikuAssistantQwen27Snapshot':'74adae678e1c2344e3c6f86bea6f8a30c80374de56cb85ac3cf090059f97cd29',
 'pioneer-q38-snapshot-rate-smoke':'fa0d81296c9a130eaba9fefed03d38033b28087d5df92ca8e672a9eb16058c65'}
for name,digest in artifacts.items():assert (build/name).is_file() and sha(build/name)==digest,name
base=Path(__file__).resolve().parent/'pioneer_q38_full_offload_vector_q4_rate_physical.sh'
assert sha(base)=='d697bfd0d2d44e37464c166b7edf2da67f6133df4dc9f6cff3ea8c02a5ada6d8'
text=base.read_text()
changes={
 '# ONE full-card vector-Q4 tag92 rate trial;fixed generation16 admission.':'# ONE snapshot tag117 rate trial;fixed generation10 admission.',
 'pioneer-q38-full-vector-q4-20261104-a':'pioneer-q38-job-snapshot-117-a',
 'tenstorrent_pci_probe-full-vector-q4-92':'tenstorrent_pci_probe-snapshot117',
 'HaikuAssistantQwen27FullOffload-vector-q4':'HaikuAssistantQwen27Snapshot',
 'client=$stage/pioneer-q38-full-offload-rate-smoke':'client=$stage/pioneer-q38-snapshot-rate-smoke',
 'candidate_hash=c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014':'candidate_hash='+artifacts['tenstorrent_pci_probe-snapshot117'],
 'worker_hash=1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9':'worker_hash='+artifacts['HaikuAssistantQwen27Snapshot'],
 'client_hash=9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af':'client_hash='+artifacts['pioneer-q38-snapshot-rate-smoke'],
 'native_tag=2026110400010092':'native_tag=2026110600010117',
 'stable_generation=16 native_generation=17 restored_generation=18':'stable_generation=10 native_generation=11 restored_generation=12'}
for old,new in changes.items():
 assert text.count(old)==1,old;text=text.replace(old,new)
with output.open('x') as f:f.write(text)
subprocess.run(['sh','-n',str(output)],check=True)
print('GENERATED only prior guarded rate flow with 10 exact identity substitutions',sha(output))
print('NO stage, device, driver, owner, benchmark or speed authorization')
