#!/usr/bin/env python3
"""Retain the entire tested physical rate lifecycle,not just its final line."""
import os,pathlib,runpy,subprocess
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_vector_q4_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_vector_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_prepared_q4_rate_physical.sh';new=path.read_text()
subprocess.run(['sh','-n',str(path)],check=True)
changes={
 'ONE full-card vector-Q4 tag92 rate trial;fixed generation16 admission':'ONE full-card prepared-Q4 tag97 rate trial;fixed generation24 admission',
 '/boot/home/pioneer-q38-full-vector-q4-20261104-a':'/boot/home/pioneer-q38-full-prepared-q4-20261105-a',
 'tenstorrent_pci_probe-full-vector-q4-92':'tenstorrent_pci_probe-full-prepared-q4-97',
 'HaikuAssistantQwen27FullOffload-vector-q4':'HaikuAssistantQwen27FullOffload-prepared-q4',
 'c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014':'f4d98a7f6c561703ec0ed9111ddaa7f263898c02e305804c2af918347643ed88',
 '1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9':'10df2a16c8fee1208743117418a5b155e986a6868c09ea237c6e0dac56a37bd0',
 '9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af':'588133ca6b05dadd1bc6b3075e08542813116bec73ec7dce0a5f23dd5afe6c0f',
 'native_tag=2026110400010092':'native_tag=2026110500010097',
 'stable_generation=16 native_generation=17 restored_generation=18':'stable_generation=24 native_generation=25 restored_generation=26'}
for source,target in changes.items():
 assert old.count(source)==1 and new.count(target)==1;old=old.replace(source,target)
assert old==new,'No other admission/exchange/actual-exit/restoration alteration permitted'
print('PASS prepared Q4 tag97 parent exact tested lifecycle except9 immutable revision substitutions;NO card or speed claim',flush=True)
