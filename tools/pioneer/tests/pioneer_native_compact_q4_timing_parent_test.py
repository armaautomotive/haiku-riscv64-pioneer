#!/usr/bin/env python3
"""Actual107 shell flow fixtures;no card or native prerequisite authority."""
import concurrent.futures
import pathlib
import re
import subprocess
import sys
import tempfile
from pioneer_native_compact_q4_timing_output import fixture as _fixture,verify as _verify,verify_parent as _verify_parent
from pioneer_native_compact_q4_stream_output import REFERENCE

assert __debug__
paired=sys.argv[1:]==['--pair-issue']
assert paired or not sys.argv[1:]
def fixture(unavailable=False):return _fixture(unavailable,paired)
def verify(text):return _verify(text,paired)
def verify_parent(text):return _verify_parent(text,paired)
before,native,after=(44,45,46) if paired else (42,43,44)
tag='2026110600010108' if paired else '2026110600010107'
HERE=pathlib.Path(__file__).resolve().parent
source=(HERE.parent/('pioneer_native_compact_q4_pair_issue_physical.sh' if paired else 'pioneer_native_compact_q4_timing_physical.sh')).read_text()
old=(HERE.parent/'pioneer_native_compact_q4_stream_physical.sh').read_text()
# Common physical ownership/metadata/exchange/idle/CPU/RAM procedures are
# exactly unchanged,not reimplemented fixture versions of those procedures.
assert source.split('exact(){',1)[1].split('test -d "$stage"',1)[0]==old.split('exact(){',1)[1].split('test -d "$stage"',1)[0]
assert source.split('# NATIVE_COMPACT_Q4_STREAM_PRIOR_GATE_BEGIN\n',1)[1].split('# NATIVE_COMPACT_Q4_STREAM_PRIOR_GATE_END',1)[0]==old.split('# NATIVE_COMPACT_Q4_STREAM_PRIOR_GATE_BEGIN\n',1)[1].split('# NATIVE_COMPACT_Q4_STREAM_PRIOR_GATE_END',1)[0]
flow=source.split('# NATIVE_COMPACT_Q4_STREAM_FLOW_BEGIN\n',1)[1].split('# NATIVE_COMPACT_Q4_STREAM_FLOW_END',1)[0]
folder=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-timed-q4-parent-'))
client=folder/'client';client.write_text('''#!/bin/sh
[ "$#" = 1 ] && [ "$1" = --fixed-compact-q4-timing-full-k-stream ] || exit 2
echo run >> "$events"
if [ "$mode" = runfail ];then exit 1;fi
cat "$stage/output"
if [ "$mode" = dirty ];then echo ERROR >&2;fi
if [ "$mode" = false-pass ];then exit 1;fi
'''.replace('--fixed-compact-q4-timing-full-k-stream','--fixed-compact-q4-pair-issue-full-k-stream' if paired else '--fixed-compact-q4-timing-full-k-stream'));client.chmod(0o700)
script=folder/'flow.sh'
setup='''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;reference=$stage/reference
export stage events mode
stable_hash=STABLE;candidate_hash=NATIVE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
mocktag=2026102100010012;mockgeneration=42
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$3" = "$mocktag" ] && [ "$5" = "$mockgeneration" ] && [ "$mode" != swapfail ] || return 1;mocktag=$4;mockgeneration=$6; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events";[ "$1" = "$mocktag" ] && [ "$2" = "$mockgeneration" ]; }
health(){ echo health >> "$events"; }
'''
if paired:setup=setup.replace('mockgeneration=42','mockgeneration=44')
script.write_text(setup+flow)
good=fixture().splitlines();cases=[('pass',good),('unavailable',fixture(True).splitlines()),('runfail',[]),
    ('dirty',good),('false-pass',good),('swapfail',good),('busy',good),('extra',good+['NOISE'])]
for index,line in enumerate(good):
    cases += [('missing'+str(index),good[:index]+good[index+1:]),('duplicate'+str(index),good[:index]+[line]+good[index:])]
    if line.startswith('NATIVE_COMPACT_Q4_STREAM_RETURNED_WORD'):
        wrong=re.sub(r'got([0-9a-f]{8}) expected\1','got00000000 expected00000000',line)
        cases.append(('wrong'+str(index),good[:index]+[wrong]+good[index+1:]))
for oldfield,newfield in [(f'generation{native}','generation37'),('polls2','polls0'),('phases4','phases1'),
    ('saved_l126908','saved_l10'),('requests1040','requests0'),('pass0 state1','pass1 state1'),
    ('state1','state3'),('cycles2000','cycles0')]:
    body=fixture().replace(oldfield,newfield);assert body!=fixture();cases.append(('field'+oldfield,body.splitlines()))
def check(item):
    number,(mode,lines)=item;stage=folder/str(number);stage.mkdir(mode=0o700)
    (stage/'reference').write_bytes(REFERENCE.read_bytes());(stage/'output').write_text('\n'.join(lines)+'\n')
    child=subprocess.run(['sh',str(script),str(stage),str(client),mode],capture_output=True)
    okay=mode in ('pass','unavailable');assert (child.returncode==0)==okay,(mode,child.stdout,child.stderr)
    events=(stage/'events').read_text() if (stage/'events').exists() else ''
    restore=f'exchange:{tag}:2026102100010012'
    assert (restore in events)==okay,(mode,events)
    if okay:
        assert (stage/'state').read_text()==('STABLE041_RESTORED_COMPACT_Q4_PAIR_ISSUE_EXACT128\n' if paired else 'STABLE041_RESTORED_COMPACT_Q4_TIMING_EXACT128\n')
        verify((stage/'compact-q4-stream.stdout').read_text())
    else:assert f'metadata:2026102100010012:{after}' not in events
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:list(pool.map(check,enumerate(cases)))
# Independent complete parent envelope,including both exact health sections.
from pioneer_native_compact_q4_stream_parent_output import example
old_lines=example().splitlines();prefix=old_lines[:18];suffix=old_lines[-18:]
def adapt(line):
    kind='PAIR_ISSUE' if paired else 'TIMING'
    return line.replace('2026110500010104',tag).replace('generation36',f'generation{before}').replace('generation37',f'generation{native}').replace('generation38',f'generation{after}').replace('NATIVE_COMPACT_Q4_STREAM_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM',f'NATIVE_COMPACT_Q4_{kind}_KNOWN_CLEAN_NO_MODEL_TPS_CLAIM').replace('COMPACT_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES_STARTED_WAIT_SAME_PARENT',f'COMPACT_Q4_{kind}_SOURCE_AUTH_AND_TWO_FIXED_BANDS_STARTED_WAIT_SAME_PARENT')
full='\n'.join([*map(adapt,prefix),*fixture().splitlines(),*map(adapt,suffix)])+'\n'
assert verify_parent(full)['restored_generation']==after
parent_bad=0
for index in range(len(full.splitlines())):
    lines=full.splitlines();bad='\n'.join(lines[:index]+lines[index+1:])+'\n'
    try:verify_parent(bad)
    except ValueError:parent_bad+=1
    else:raise AssertionError('partial parent accepted')
print(f'PASS_TIMED_Q4_ACTUAL_PARENT cases{len(cases)} fullParentMissingRefusals{parent_bad} unavailableStillRestores1 unknownNeverRestores1 helpersUnchanged1;NO_HARDWARE_AUTHORITY')
