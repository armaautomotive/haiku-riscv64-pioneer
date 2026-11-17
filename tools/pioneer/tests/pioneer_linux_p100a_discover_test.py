#!/usr/bin/env python3
"""Synthetic sysfs inventory only; never touches hardware."""
import os
from pathlib import Path
import subprocess
import tempfile

script = Path(__file__).resolve().parents[1] / 'pioneer_linux_p100a_discover.sh'
with tempfile.TemporaryDirectory(prefix='pioneer-pci-discover-', dir='/private/tmp') as tmp:
    base = Path(tmp)
    sysfs = base / 'sysfs'
    sysfs.mkdir()
    bindir = base / 'bin'
    bindir.mkdir()
    fake = bindir / 'lspci'
    fake.write_text('#!/bin/sh\nprintf "FAKE_LSPCI %s\\n" "$*"\n')
    fake.chmod(0o755)
    env = {**os.environ, 'PATH': str(bindir) + ':' + os.environ['PATH']}

    def run():
        return subprocess.run(['sh', str(script), '--sysfs-test-root', str(sysfs)],
                              capture_output=True, text=True, env=env, check=False)

    def device(name, vendor='0x1e52', dev='0xb140', sub='0x0043'):
        path = sysfs / name
        path.mkdir()
        for key, value in {'vendor': vendor, 'device': dev,
                           'subsystem_vendor': '0x1e52', 'subsystem_device': sub,
                           'class': '0x120000', 'enable': '0',
                           'current_link_speed': '16.0 GT/s', 'current_link_width': '8',
                           'resource': '0 0 0'}.items():
            (path / key).write_text(value + '\n')
        return path

    result = run()
    assert result.returncode == 1 and 'matches=0' in result.stdout and 'FAKE_LSPCI' not in result.stdout
    device('0002:81:00.0', vendor='0x1002', dev='0x6779')
    assert 'matches=0' in run().stdout
    card = device('0001:41:00.0')
    result = run()
    assert result.returncode == 0 and 'bdf=0001:41:00.0' in result.stdout
    assert 'FAKE_LSPCI -D -s 0001:41:00.0 -vv' in result.stdout
    assert 'current_link_speed 16.0 GT/s' in result.stdout and 'NO_PCI_WRITES' in result.stdout
    (card / 'subsystem_device').write_text('0x9999\n')
    result = run()
    assert result.returncode == 1 and 'unexpected_subsystem' in result.stderr and 'FAKE_LSPCI' not in result.stdout
    (card / 'subsystem_device').write_text('0x0043\n')
    device('0000:01:00.0')
    result = run()
    assert result.returncode == 1 and 'matches=2' in result.stdout and 'FAKE_LSPCI' not in result.stdout
print('PASS dynamic PCI inventory: zero, wrong GPU, unique relocated card, wrong SKU, duplicate; synthetic sysfs only')
