#!/usr/bin/env python3
"""Actual NEW one-shot BFP8 parent flow;fake commands,never SSH/MMIO."""
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
source = (here.parent / 'pioneer_native_bfp8_observe_physical.sh').read_text()
flow = source.split('# NATIVE_BFP8_OBSERVE_FLOW_BEGIN\n', 1)[1].split('# NATIVE_BFP8_OBSERVE_FLOW_END')[0]
assert '428e7a15ad97501eb711dc6a2e59f30c68d14c1b022acc32b9b753ce793aefb4' in source
assert 'KNOWN_CLEAN_CACHED_FOUR_TOKEN_WORK_REQUEST_COMPLETED' in source
assert 'cat "$stage/fixed8.stderr"' in flow and 'fixed4' not in flow and 'sed -n' not in flow
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-bfp8-observe-parent.'))
client = temporary / 'client'
client.write_text('''#!/bin/sh
echo run8 >> "$events"
if [ "$mode" = fail ];then
 for pass in 0 1;do row=0;while [ "$row" -lt 32 ];do
  echo "NATIVE_BFP_RETURNED_WORD bits8 pass$pass row$row gotSYNTHETIC expectedSYNTHETIC" >&2;row=$((row+1))
 done;done;exit 1
fi
if [ "$mode" = unknown ];then echo RUN_UNPROVEN >&2;exit 1;fi
if [ "$mode" = malformed ];then echo WRONG_SUCCESS;exit 0;fi
if [ "$mode" = dirty ];then echo UNEXPECTED_STDERR >&2;fi
echo 'PASS_NATIVE_BFP_FIXED_KERNEL bits8 rows32 dual_jobs2 exact_words64 polls1 generation3;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
''')
client.chmod(0o700)
prefix = '''set -eu
stage=$1;client=$2;mode=$3;events=$stage/events;export events mode
stable_hash=STABLE;candidate_hash=CANDIDATE;client_hash=CLIENT;installed=installed;stable=stable;candidate=candidate
exchange(){ echo "exchange:$3:$4" >> "$events";[ "$mode" != swap-fail ]; }
exact(){ :; }
idle(){ echo idle >> "$events";[ "$mode" != busy ]; }
metadata(){ echo "metadata:$1:$2" >> "$events"; }
health(){ echo health >> "$events"; }
'''
program = temporary / 'flow.sh'
program.write_text(prefix + flow)
for mode in ('pass', 'fail', 'unknown', 'malformed', 'dirty', 'swap-fail', 'busy'):
    stage = temporary / mode
    stage.mkdir()
    result = subprocess.run(['sh', str(program), str(stage), str(client), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if mode == 'pass':
        assert result.returncode == 0 and events.count('run8') == 1
        assert 'exchange:2026110300010078:2026102100010012' in events
        assert (stage / 'state').read_text().strip() == 'STABLE041_RESTORED_FIXED_BFP8_STRICT_IDEAL_PASSED'
    else:
        assert result.returncode != 0 and events.count('run8') == int(mode not in ('swap-fail', 'busy'))
        assert 'exchange:2026110300010078:2026102100010012' not in events
        if 'run8' in events:
            assert events[-1] == 'run8' # NO metadata/second GPU run/restore after failure
        if mode == 'fail':
            assert result.returncode == 70 and result.stdout.count('NATIVE_BFP_RETURNED_WORD') == 64
            assert (stage / 'fixed8.stderr').read_text().count('NATIVE_BFP_RETURNED_WORD') == 64
idle_body = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
inventory_program = temporary / 'idle.sh'
inventory_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle_body + '\n}\nidle\n')
normal = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-native-bfp8-observe-20261104-a/pioneer_native_bfp8_observe_physical.sh\n'
inventory_cases = ((normal, True), (normal + '55 /boot/system/Tracker\n', True),
                   (normal + '55 /bin/bash -c stage=/boot/home/pioneer-native-bfp8-observe-20261104-a\n', False),
                   (normal + '55 /boot/home/config/non-packaged/servers/HaikuAssistantQwen27\n', False),
                   (normal + '55 /boot/home/config/non-packaged/servers/HaikuAssistantService\n', False),
                   (normal + '55 /boot/home/pioneer-p100a-wide-gdn-test\n', False),
                   ('Id Team\n', False), (normal + 'malformed\n', False), (normal + 'Id Team\n', False))
for index, (listing, accepted) in enumerate(inventory_cases):
    inventory = temporary / ('inventory-' + str(index))
    inventory.write_text(listing)
    check = subprocess.run(['sh', str(inventory_program), str(inventory)], text=True, capture_output=True)
    assert (check.returncode == 0) == accepted
print('PASS actual new BFP8 observation parent:7 fake flow+9 actual idle-parser cases;one BFP8 only,all64 failed words preserved,unknown/malformed/dirty/swap/busy failures NO retry/metadata/restore,strict success only known-clean restoration;NO physical proof')
