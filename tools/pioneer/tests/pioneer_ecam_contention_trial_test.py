#!/usr/bin/env python3
"""Exercise the actual trial wrapper's cleanup and fault retention on fake files."""
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
original = (repo / 'tools/pioneer/pioneer_ecam_lease_probe_trial.sh').read_text()


def put(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(0o755)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


for scenario in ('success', 'clean_failure', 'retained_failure', 'root_ready'):
    with tempfile.TemporaryDirectory(prefix='q35-contention-trial-') as temporary:
        root = Path(temporary)
        stage = root / 'stage'
        stage.mkdir()
        home = root / 'home'
        bin_dir = home / 'config/non-packaged/add-ons/kernel/drivers/bin'
        dev_dir = home / 'config/non-packaged/add-ons/kernel/drivers/dev/misc'
        bin_dir.mkdir(parents=True)
        dev_dir.mkdir(parents=True)
        device = root / 'dev/pioneer_ecam_lease_probe'
        device.parent.mkdir()
        put(stage / 'pioneer_ecam_lease_probe', 'fake driver\n')
        put(stage / 'ecam_lease_probe_client', '#!/bin/sh\n' +
            ('test "$1" = --root-reset-ready\n' if scenario == 'root_ready' else 'exit 0\n'))
        put(stage / 'ecam_lease_contention_client', '#!/bin/sh\nexit ' +
            {'success': '0', 'clean_failure': '1', 'retained_failure': '2', 'root_ready': '0'}[scenario] + '\n')
        admin = home / 'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        put(admin, '#!/bin/sh\necho "P100A_RELOAD driver55 tag2026102100010012 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY"\n')
        mock = root / 'mock'
        put(mock / 'uname', '#!/bin/sh\necho Haiku\n')
        put(mock / 'id', '#!/bin/sh\necho 0\n')
        put(mock / 'ps', '#!/bin/sh\nprintf "Id Team\\n1 kernel_team\\n"\n')
        put(mock / 'listimage', '#!/bin/sh\nexit 0\n')
        put(mock / 'sha256sum', '''#!/usr/bin/env python3
import hashlib,sys
p=sys.argv[1]
print(hashlib.sha256(open(p,'rb').read()).hexdigest(),p)
''')
        put(mock / 'ln', '#!/bin/sh\n/bin/ln "$@"\n/usr/bin/touch "$MOCK_DEVICE"\n')
        put(mock / 'rm', '#!/bin/sh\n/bin/rm "$@"\n/bin/rm -f "$MOCK_DEVICE"\n')
        script = original.replace('/boot/home', str(home)).replace(
            'device=/dev/misc/pioneer_ecam_lease_probe', 'device=' + str(device))
        pins = {
            '710bdb640fb096d899447f58d6b25e7d16ea075d8a1fa4765e3baf77cfd2b346': stage / 'pioneer_ecam_lease_probe',
            '2d9842a8b21b3fcb860f18efc223cd0c860332b633d0a6d4c1be78b468c10c77': stage / 'ecam_lease_probe_client',
            '25a04b825db7f6390026cc4dd34780e4b9c36ce4156de441bf9aee95f31e8de4': stage / 'ecam_lease_contention_client',
            'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f': admin,
        }
        for expected, path in pins.items():
            assert expected in script
            script = script.replace(expected, digest(path))
        wrapper = root / 'trial.sh'
        put(wrapper, script)
        env = dict(os.environ, PATH=str(mock) + ':' + os.environ['PATH'], MOCK_DEVICE=str(device))
        mode = 'root-ready' if scenario == 'root_ready' else 'contention'
        result = subprocess.run(['bash', str(wrapper), str(stage), '1', mode], env=env,
                                text=True, capture_output=True, timeout=15)
        lock = home / '.pioneer-q38-head-csr-exclusive'
        installed = bin_dir / 'pioneer_ecam_lease_probe'
        link = dev_dir / 'pioneer_ecam_lease_probe'
        if scenario in ('success', 'root_ready'):
            assert result.returncode == 0, (result.stdout, result.stderr)
            assert not lock.exists() and not installed.exists() and not link.is_symlink() and not device.exists()
        elif scenario == 'clean_failure':
            assert result.returncode != 0 and 'CONTENTION_PROBE_RETAINED' not in result.stderr
            assert not lock.exists() and not installed.exists() and not link.is_symlink() and not device.exists()
        else:
            assert result.returncode != 0 and 'CONTENTION_PROBE_RETAINED' in result.stderr
            assert lock.is_dir() and installed.is_file() and link.is_symlink() and device.exists(), (
                result.stdout, result.stderr, lock.is_dir(), installed.is_file(), link.is_symlink(), device.exists())
print('PASS contention and root-ready wrapper cleanup and fault retention; SIMULATED FILESYSTEM ONLY')
