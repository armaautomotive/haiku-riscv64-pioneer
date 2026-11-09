#!/usr/bin/env python3
"""Exercise the actual one-shot physical parent flow using fake endpoints."""
import os
from pathlib import Path
import subprocess
import tempfile

if not __debug__:
    raise SystemExit('Assertions required')
here=Path(__file__).resolve().parent
source=(here.parent/'pioneer_native_srca_probe_physical_once.sh').read_text()
flow=source[source.index('exchange "$stable_hash" "$candidate_hash"'):]
with tempfile.TemporaryDirectory(prefix='pioneer-srca-parent-',dir='/private/tmp') as folder:
    root=Path(folder);client=root/'fake-client'
    client.write_text('''#!/bin/sh
[ "$1" = --fixed-srca-probe ] || exit 2
echo run >> "$events"
if [ "$mode" = fail ];then echo FIXTURE_UNKNOWN >&2;exit 70;fi
if [ "$mode" = bad ];then echo WRONG_COMPLETION;exit 0;fi
if [ "$mode" = stderr ];then echo UNEXPECTED >&2;fi
echo 'PASS_NATIVE_SRCA_PROBE_FIXED old_shift32 corrected32 polls1 generation15;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_OR_SPEED_CLAIM'
''')
    client.chmod(0o700)
    prefix='''#!/bin/sh
set -eu
stage=$1;client=$2;events=$3;mode=$4;export events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;installed=installed;candidate=candidate;stable=stable
exchange(){ echo "exchange:$3:$4" >> "$events"; }
exact(){ :; }
idle(){ :; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
sysinfo(){ echo FIXTURE_SYSINFO; }
'''
    program=root/'flow.sh';program.write_text(prefix+flow)
    for mode in ('pass','fail','bad','stderr'):
        stage=root/mode;stage.mkdir();events=stage/'events'
        result=subprocess.run(['sh',str(program),str(stage),str(client),str(events),mode],text=True,capture_output=True)
        sequence=events.read_text().splitlines()
        assert sequence.count('run')==1
        restore='exchange:2026110900010135:2026102100010012'
        if mode=='pass':
            assert result.returncode==0 and restore in sequence
            assert (stage/'state').read_text().strip()=='STABLE041_RESTORED_FIXED_SRCA_PASSED'
        else:
            assert result.returncode!=0 and restore not in sequence
            assert sequence[-1]=='run',sequence
            if mode=='fail':assert (stage/'state').read_text().strip()=='UNKNOWN_FIXED_SRCA;NO_END_RESET_RETRY_RESTORE'
print('PASS actual SrcA parent flow: success restores once; failure/malformed output never retries or restores; FAKE endpoints')
