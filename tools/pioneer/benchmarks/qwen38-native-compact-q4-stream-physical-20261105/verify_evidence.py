#!/usr/bin/env python3
"""Verify historical scoped104 records,not current device/exit/admission authority."""
import hashlib
import json
import pathlib
import sys
assert __debug__ and len(sys.argv)==1
here=pathlib.Path(__file__).resolve().parent;root=here.parents[3]
m=json.loads((here/'manifest.json').read_text())
assert m['version']==1 and m['spent'] is True and m['restored_generation']==38
assert len(m['files'])==12 and len(m['readers'])==4
assert m['terminal']=={'outer':{'session':73746,'exit':0},'collection':{'session':8675,'exit':0},'postflight':{'session':7457,'exit':0}}
for name,pin in m['files'].items():
 p=pathlib.PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
 data=(here/p).read_bytes();assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
 if name.endswith('.stderr'):assert not data
for name,pin in m['readers'].items():
 p=pathlib.PurePosixPath(name);assert not p.is_absolute() and '..' not in p.parts
 assert hashlib.sha256((root/p).read_bytes()).hexdigest()==pin,name
assert m['readers']['tools/pioneer/pioneer_native_compact_q4_stream_physical.sh']=='fb8949d50e06ab69cfc921369286ad072fe1c60fbd210762188cf8e7e8f0608a'
sys.path.insert(0,str(root/'tools/pioneer/tests'))
from pioneer_native_compact_q4_stream_output import verify
from pioneer_native_compact_q4_stream_parent_output import verify_parent,health,metadata
# Preserve raw byte framing;read_text() would normalize CRLF before checking.
child=verify((here/'compact-q4-stream.stdout').read_bytes().decode('ascii'))
parent=verify_parent((here/'parent.stdout').read_bytes().decode('ascii'))
assert child=={'samples':9,'words':576,'ends':9,'generation':37,'polls':[1]*9}
assert parent==dict(child,restored_generation=38)
for name in ('parent.exit','compact-q4-stream.exit'):assert (here/name).read_bytes()==b'0\n'
assert (here/'parent.pid').read_bytes()==b'32948\n' and (here/'wrapper.pid').read_bytes()==b'32941\n'
assert m['parent_pid']==32948 and m['wrapper_pid']==32941
assert (here/'state').read_bytes()==b'STABLE041_RESTORED_COMPACT_Q4_STREAM_EXACT576_PASSED\n'
raw=(here/'postflight.stdout').read_bytes();assert raw.endswith(b'\n') and b'\r' not in raw and b'\0' not in raw
lines=raw.decode('ascii').splitlines();assert len(lines)==11
assert lines[0]==metadata('2026102100010012',38)
health(lines[1:10])
assert lines[1]==(here/'parent.stdout').read_bytes().decode('ascii').splitlines()[1]
assert lines[-1]=='INDEPENDENT_COMPACT104_POSTFLIGHT_STABLE041_GEN38_PARENT_AND_WRAPPER_GONE_CPU64_FULL_RAM_ASSISTANT_UNCHANGED_SPENT'
script=(here/'postflight.sh').read_text()
for witness in ('idle','exact "$installed" "$stable_hash"'):
 assert witness in script
assert 'awk \'$1==32948 || $1==32941 {live++} END{if(live)exit 1}\'' in script
assert m['normal_sha256']=='041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
print('PASS FROZEN bounded REAL104 records:576 exact returned words/nine clean ENDs/strict624 parent records/recorded outer0+postflight0/normal041 gen38/64CPU fullRAM;SPENT,NOT current hardware admission/full-model quality/tokens-sec proof')
