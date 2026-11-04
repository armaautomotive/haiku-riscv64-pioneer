#!/usr/bin/env python3
"""Actual CLOSED86 restore-only flow/strict epoch/regression fixtures,NO GPU."""
import pathlib
import re
import subprocess
import tempfile
assert __debug__
here=pathlib.Path(__file__).resolve().parents[1]
source=(here/'pioneer_native_q4_fpu_stream_closed_restore.sh').read_text()
old=(here/'pioneer_native_q4_fpu_stream_physical.sh').read_text()
for forbidden in ('--fixed-q4-fpu-full-k-stream','--reset','pioneer_power','exchange(){'):
    assert forbidden not in source
assert '7588578d8d49a8e65c62c0277fe9f4dc46f3c0186a4ef6abd2bed811b439d347' in source
assert '178f86c96b85a1df0e4bc65d97c618069d2b3b0efee0bd75b91db697b18812ff' in source
assert 'RUNNING_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES' in source
assert re.findall(r'metadata (2026[0-9]+) ([0-9]+)',source)==[
    ('2026110400010086','9'),('2026110400010086','9'),('2026102100010012','10')]
assert source.count('"$admin" --quiesce 2026110400010086')==1
assert source.count('"$admin" --wait 2026102100010012 0000000000000009')==1
flow=source.split('# NATIVE_Q4_CLOSED_RESTORE_FLOW_BEGIN\n',1)[1].split('# NATIVE_Q4_CLOSED_RESTORE_FLOW_END',1)[0]
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-closed86-restore.'))
program=folder/'flow.sh'
program.write_text('''set -eu
stage=$1;failure=$2;stable=$stage/stable;installed=$stage/installed;stable_hash=STABLE;candidate_hash=NATIVE;admin=Admin
step(){ n=$(cat "$stage/count");n=$((n+1));echo "$n" > "$stage/count";echo "$1" >> "$stage/events";[ "$n" != "$failure" ]; }
exact(){ step exact; };idle(){ step idle; };metadata(){ step "metadata:$1:$2"; };health(){ step health; };Admin(){ step "admin:$1:$2"; }
mkdir(){ step mkdir && command mkdir "$@"; };printf(){ step printf && command printf "$@"; }
cp(){ step cp && command cp "$@"; };chmod(){ step chmod && command chmod "$@"; }
mv(){ step mv && command mv "$@"; };sync(){ step sync; };echo(){ command echo "$@"; }
'''+flow)
count=None
for failure in range(100):
    stage=folder/str(failure);stage.mkdir();(stage/'count').write_text('0\n');(stage/'stable').write_text('STABLE');(stage/'installed').write_text('NATIVE')
    r=subprocess.run(['sh',str(program),str(stage),str(failure)],text=True,capture_output=True);events=(stage/'events').read_text().splitlines()
    if not failure:
        assert r.returncode==0 and (stage/'installed').read_text()=='STABLE'
        assert events.count('admin:--quiesce:2026110400010086')==1 and events.count('admin:--wait:2026102100010012')==1
        assert (stage/'restore.state').read_text().strip()=='STABLE041_GENERATION10_RESTORED_AFTER_NINE_CLEAN_ENDS_ORIGINAL_PARENT_FAILED'
        count=len(events)
    else:assert r.returncode and len(events)==failure,(failure,events,r.stdout,r.stderr)
    if failure==count:break
assert count and count<100
metadata=source.split('metadata(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'metadata.sh';program.write_text('set -eu\nadmin=Admin;value_text=$1\nAdmin(){ command printf "%s\\n" "$value_text"; }\nmetadata(){\n'+metadata+'\n}\nmetadata 2026110400010086 9\n')
normal='P100A_RELOAD driver55 tag2026110400010086 generation9 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
variants=[normal]
for a,b in [('generation9','generation7'),('generation9','generation8'),('generation9','generation10'),('cold0','cold1'),('active_owner0','active_owner1'),
            ('quiesced0','quiesced1'),('open_count1','open_count2'),('guard1;','guard0;'),('0086','0085')]:variants.append(normal.replace(a,b))
for i,text in enumerate(variants):
    r=subprocess.run(['sh',str(program),text],capture_output=True);assert (r.returncode==0)==(i==0)
# This regression intentionally identifies the SPENT parent defect rather
# than modifying,rerunning or retrospectively promoting that parent.
tail=old.split('# NATIVE_Q4_STREAM_OUTPUT_GATE_END\n',1)[1]
m=re.match(r'metadata (\d+) (\d+) \|\| exit 1',tail);assert m and m.groups()==('2026110400010086','7')
old_metadata=old.split('metadata(){\n',1)[1].split('\n}\n',1)[0]
program=folder/'stale-parent.sh';program.write_text('set -eu\nadmin=Admin\nAdmin(){ command printf "%s\\n" "'+normal+'"; }\nmetadata(){\n'+old_metadata+'\n}\nmetadata '+m[1]+' '+m[2]+' || exit 1\n')
r=subprocess.run(['sh',str(program)],capture_output=True);assert r.returncode==1 and r.stdout.decode().strip()==normal
print(f'PASS actual CLOSED86 restore:{count+1} first-failure flow+{len(variants)} strict epoch/owner/cold cases;SPENT parent stale7-vs-real9 regression reproduced;literal9->10 transitions checked;NO GPU/RUN/END/reset/retry/admission/old-parent-pass authority')
print('CPU-only closed86 restore evidence:',folder)
