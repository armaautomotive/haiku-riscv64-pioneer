#!/usr/bin/env python3
"""Exercise the actual recovery shell with synthetic files and commands only."""
import os
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parent.parent / 'pioneer_qgw8_postoutage_restore.sh').read_text()
shim = r'''
uname(){ echo Haiku; }
id(){ echo 0; }
stat(){ case "$2" in '%u:%a') echo 0:700;; '%u:%h:%a') echo 0:1:600;; '%d') echo 1;; *) return 1;; esac; }
sysinfo(){ if [ "$MODE" = health ]; then echo bad; else echo '64 CPUs running at speed'; echo 'used/max 1 / 137034936320)'; fi; }
ps(){ echo 'Id Team'; if [ "$MODE" = busy ]; then echo '99 99 /boot/system/Assistant'; else echo '99 99 harmless'; fi; }
sha256sum(){
 case "${1##*/}" in
  state) h=75e078802fd23010cf3d8e76e2fe9f72410fd9ec1779a9b9afa9f2824b7a1660;;
  parent.stdout) h=b09b7f8e1b55da9bbd282ca13fa99edebe836b0ac6a3e51b75280a5adeae4ba2;;
  parent.stderr|trial.stderr) h=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855;;
  trial.stdout) h=628efcfc3f4a11a3213f9f9286a95488c82faf9206c9e6abb8ee257bcee7b0ab;;
  tenstorrent_pci_probe|previous129) case "$(cat "$1")" in OLD) h=0b61c939d9b461a6c60e0d7f68a2c3bccac588ea052dc634d85b9a35d32b929f;; NEW) h=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;; *) h=bad;; esac;;
  tenstorrent_pci_probe-qgw8-hidden-129) h=0b61c939d9b461a6c60e0d7f68a2c3bccac588ea052dc634d85b9a35d32b929f;;
  stable.backup|incoming041) h=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403;;
  pioneer-p100a-reload) h=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f;;
  pioneer_p100a_reload_guard) h=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229;;
  checked-flush) h=242400b3c143573ad6c8bd890e12114de4c1573b86d70a59499b0ed61227e90d;;
  *) return 1;;
 esac
 [ "$MODE:${1##*/}" != record:state ] || h=bad
 printf '%s  %s\n' "$h" "$1"
}
'''

with tempfile.TemporaryDirectory(prefix='qgw8-postoutage-test-') as tmp:
    for mode in ('pass', 'check', 'bad_backup', 'no_epoch', 'record', 'exit_present',
                 'snapshot_present', 'spent', 'busy', 'health', 'owner', 'wrong_tag',
                 'wrong_generation', 'flush1', 'quiesce', 'changed', 'flush2', 'wait'):
        root = Path(tmp) / mode / 'home'
        stage = root / 'pioneer-qgw8-hidden-trial-20261108-a'
        stage.mkdir(parents=True)
        (stage / '.trial-once').mkdir()
        for name in ('state', 'parent.stdout', 'parent.stderr', 'trial.stdout', 'trial.stderr',
                     'tenstorrent_pci_probe-qgw8-hidden-129'):
            (stage / name).write_text('OLD')
        (stage / 'stable.backup').write_text('NEW')
        if mode == 'exit_present': (stage / 'trial.exit').touch()
        if mode == 'snapshot_present': (stage / 'hidden.bin').touch()
        work = root / 'pioneer-qgw8-postoutage-restore-20261108-a'
        if mode == 'spent': work.mkdir()
        installed = root / 'config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe'
        installed.parent.mkdir(parents=True); installed.write_text('OLD')
        device = root / 'config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe'
        device.parent.mkdir(parents=True); device.symlink_to('../../bin/tenstorrent_pci_probe')
        guard = root / 'config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard'
        guard.parent.mkdir(parents=True); guard.touch()
        admin = root / 'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.parent.mkdir(parents=True)
        admin.write_text('''#!/bin/sh
echo "$1" >> "$EVENTS"
case "$1" in
 --status)
  tag=2026110700010129; generation=1; owner=0
  if [ "$(cat "$INSTALLED")" = NEW ]; then tag=2026102100010012; generation=2; fi
  case "$MODE" in owner) owner=1;; wrong_tag) tag=bad;; wrong_generation) generation=9;; esac
  echo "P100A_RELOAD driver55 tag${tag} generation${generation} quiesced0 cold0 active_owner${owner} open_count1 guard1;CPU_METADATA_ONLY";;
 --quiesce) if [ "$MODE" = changed ]; then echo BAD > "$INSTALLED"; fi; [ "$MODE" != quiesce ];;
 --wait) [ "$MODE" != wait ];;
esac
'''); admin.chmod(0o700)
        flush = root / 'pioneer-qgw8-flush-tool-20261107-a/checked-flush'
        flush.parent.mkdir(parents=True)
        flush.write_text('''#!/bin/sh
echo --flush >> "$EVENTS"
count=$(/usr/bin/grep -c '^--flush$' "$EVENTS")
[ "$MODE" != "flush$count" ]
'''); flush.chmod(0o700)
        program = Path(tmp) / mode / 'run.sh'
        program.write_text(source.replace('set -eu\n', 'set -eu\n' + shim, 1)
                           .replace('PATH=/boot/system/bin:/bin', 'PATH=/usr/bin:/bin')
                           .replace('/boot/home', str(root)))
        events = Path(tmp) / mode / 'events'; events.touch()
        args = ['sh', str(program), '--check-only' if mode == 'check' else '--restore',
                '--attest-mac-backup', 'bad' if mode == 'bad_backup' else 'a'*64,
                'bad' if mode == 'no_epoch' else '--attest-fresh-power-cycle']
        result = subprocess.run(args, env={**os.environ, 'MODE': mode, 'EVENTS': str(events),
                                          'INSTALLED': str(installed)}, capture_output=True, text=True)
        calls = events.read_text().splitlines()
        assert (stage / '.trial-once').is_dir() and (stage / 'state').read_text() == 'OLD'
        if mode == 'check':
            assert result.returncode == 0 and calls == ['--status'] and not work.exists(), (mode, result.stderr, calls)
        elif mode == 'pass':
            assert result.returncode == 0, (mode, result.stdout, result.stderr, calls)
            assert calls == ['--status', '--flush', '--quiesce', '--flush', '--wait', '--status']
            assert installed.read_text() == 'NEW' and (work / 'previous129').read_text() == 'OLD'
        else:
            assert result.returncode != 0, (mode, result.stdout, calls)
            if mode in ('bad_backup', 'no_epoch', 'record', 'exit_present', 'snapshot_present',
                        'spent', 'busy', 'health', 'owner', 'wrong_tag', 'wrong_generation'):
                assert '--quiesce' not in calls and '--flush' not in calls and installed.read_text() == 'OLD', (mode, calls)
            if mode == 'flush1': assert calls == ['--status', '--flush'] and installed.read_text() == 'OLD'
            if mode == 'quiesce': assert calls == ['--status', '--flush', '--quiesce'] and installed.read_text() == 'OLD'
            if mode == 'changed': assert calls == ['--status', '--flush', '--quiesce'] and installed.read_text().strip() == 'BAD'
            if mode == 'flush2': assert calls == ['--status', '--flush', '--quiesce', '--flush'] and installed.read_text() == 'NEW'
            if mode == 'wait': assert calls == ['--status', '--flush', '--quiesce', '--flush', '--wait'] and installed.read_text() == 'NEW'
print('PASS actual post-outage recovery flow and 16 synthetic refusal/fault cases; NO device')
