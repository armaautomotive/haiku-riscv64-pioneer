#!/usr/bin/env python3
"""Actual two-file deployment flow with fake files/commands;NO SSH/device use."""
import pathlib
import subprocess
import tempfile
if not __debug__:
    raise SystemExit('Assertions required')
source = (pathlib.Path(__file__).resolve().parent.parent / 'install_assistant_lossless_cache.sh').read_text()
assert '316e1655edfac771e3edbda7f7a2408385ec44676c0d0a7e443d730ec38d6a2a' in source
assert 'KNOWN_CLEAN_CACHED_FOUR_TOKEN_WORK_REQUEST_COMPLETED' in source
assert all(word not in source for word in ('--quiesce', '--wait ', 'kill ', 'trap ', 'rm ', 'reboot', 'shutdown'))
flow = source.split('# CACHE_INSTALL_FLOW_BEGIN', 1)[1].split('\n', 1)[1].split('# CACHE_INSTALL_FLOW_END')[0]
temporary = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-cache-install-flow.'))
prefix = '''set -eu
stage=$1;mode=$2
worker=$stage/worker;service=$stage/service;newworker=$stage/newworker;newservice=$stage/newservice
worker_old=OLDW;worker_new=NEWW;service_old=OLDS;service_new=NEWS
events=$stage/events
exact(){
 printf 'exact:%s:%s\\n' "${1##*/}" "$2" >> "$events"
 [ "$(cat "$1")" = "$2" ] || return 1
 if [ "$mode" = before-bad ] && [ "$1" = "$stage/before/HaikuAssistantService" ];then return 1;fi
 if [ "$mode" = staged-bad ] && [ "$1" = "$service.p100a-lossless-20261103-new" ];then return 1;fi
 if [ "$mode" = final-bad ] && [ "$1" = "$service" ] && [ "$2" = NEWS ];then return 1;fi
}
idle(){ printf 'idle\\n' >> "$events";[ "$mode" != busy ]; }
metadata(){ printf 'metadata\\n' >> "$events";[ "$mode" != cold ]; }
mv(){
 printf 'mv:%s\\n' "${2##*/}" >> "$events"
 if [ "$mode" = second-mv-fail ] && [ "$2" = "$service" ];then return 1;fi
 command mv "$@"
}
'''
program = temporary / 'flow.sh'
program.write_text(prefix + flow)
for mode in ('pass', 'before-bad', 'staged-bad', 'busy', 'cold', 'second-mv-fail', 'final-bad'):
    stage = temporary / mode
    stage.mkdir(mode=0o700)
    for name, value in (('worker', 'OLDW'), ('service', 'OLDS'), ('newworker', 'NEWW'), ('newservice', 'NEWS')):
        (stage / name).write_text(value)
    result = subprocess.run(['sh', str(program), str(stage), mode], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    moves = [event for event in events if event.startswith('mv:')]
    assert (stage / '.install-once').is_dir()
    assert (stage / 'before/HaikuAssistantQwen27').read_text() == 'OLDW'
    assert (stage / 'before/HaikuAssistantService').read_text() == 'OLDS'
    if mode == 'pass':
        assert result.returncode == 0 and moves == ['mv:worker', 'mv:service']
        assert (stage / 'worker').read_text() == 'NEWW' and (stage / 'service').read_text() == 'NEWS'
        assert (stage / 'state').read_text().strip() == 'CACHED_WORKER_AND_BOUNDED_LOAD_SERVICE_INSTALLED_NOT_STARTED'
    else:
        assert result.returncode != 0
        assert moves == (['mv:worker', 'mv:service'] if mode in ('second-mv-fail', 'final-bad') else [])
        assert (stage / 'worker').read_text() == ('NEWW' if moves else 'OLDW')
        assert (stage / 'service').read_text() == ('NEWS' if mode == 'final-bad' else 'OLDS')
    # Spent marker refuses a second flow;no second replacement/backup/rollback.
    again = subprocess.run(['sh', str(program), str(stage), mode], text=True, capture_output=True)
    assert again.returncode != 0 and (stage / 'events').read_text().splitlines() == events
idle_body = source.split('idle(){\n', 1)[1].split('\n}\n', 1)[0]
idle_program = temporary / 'idle.sh'
idle_program.write_text('set -eu\ninventory=$1\nps(){ sed "s/SELF/$$/g" "$inventory"; }\nidle(){\n' + idle_body + '\n}\nidle\n')
normal = 'Id Team\n1 kernel_team\nSELF /boot/home/pioneer-assistant-cache-installed-20261103-a/install_assistant_lossless_cache.sh --apply\n'
inventories = ((normal, True), (normal + '92 /boot/system/bin/Tracker\n', True),
               (normal + '92 /bin/bash -c stage=/boot/home/pioneer-assistant-cache-installed-20261103-a\n', False),
               (normal + '92 /boot/home/config/non-packaged/servers/HaikuAssistantService\n', False),
               (normal + '92 /boot/home/config/non-packaged/servers/HaikuAssistantQwen27\n', False),
               (normal + '92 /boot/home/pioneer-native-bfp-diagnostic\n', False),
               ('Id Team\n', False), (normal + 'malformed\n', False), (normal + 'Id Team\n', False))
for index, (listing, accepted) in enumerate(inventories):
    inventory = temporary / ('inventory-' + str(index))
    inventory.write_text(listing)
    check = subprocess.run(['sh', str(idle_program), str(inventory)], text=True, capture_output=True)
    assert (check.returncode == 0) == accepted
print('PASS actual cache install:7 fake flow+9 actual idle-parser cases;backups/preparation before mutation,worker-first,failed gates/replacement NO retry/rollback/start/GPU,spent-token refusal;real owners and path-bearing wrapper rejected,self direct exec accepted')
