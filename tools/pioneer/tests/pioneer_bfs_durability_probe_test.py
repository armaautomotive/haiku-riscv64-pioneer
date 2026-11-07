#!/usr/bin/env python3
"""Exercise only the fake-flush host build in isolated temporary storage."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-bfs-probe-test-',dir='/private/tmp') as tmp:
    parent=Path(tmp);binary=parent/'probe';storage=parent/'storage';storage.mkdir(mode=0o700)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-Werror',
                    '-DPIONEER_BFS_PROBE_TEST=1',str(here.parent/'pioneer_bfs_durability_probe.cpp'),'-o',str(binary)],check=True)
    env={**os.environ,'PIONEER_BFS_PROBE_TEST_PARENT':str(storage)}
    def run(*args,ok=True,extra=None):
        result=subprocess.run([str(binary),*args],capture_output=True,text=True,env={**env,**(extra or {})})
        assert (result.returncode==0)==ok,(args,result.stdout,result.stderr)
        return result
    assert not list(storage.iterdir())
    run('--create','/dev/wrong',ok=False);assert not list(storage.iterdir())
    made=run('--create','/dev/pioneer-test')
    stage=Path(next(line[6:] for line in made.stdout.splitlines() if line.startswith('STAGE ')))
    assert stage.parent==storage and len(list(stage.iterdir()))==4
    run('--verify',str(stage),'/dev/pioneer-test')
    run('--verify',str(stage),'/dev/wrong',ok=False)
    run('--verify',str(stage)+'/..','/dev/pioneer-test',ok=False)
    target=stage/'small41';original=target.read_bytes();target.write_bytes(bytes([original[0]^1])+original[1:])
    run('--verify',str(stage),'/dev/pioneer-test',ok=False);target.write_bytes(original)
    target.chmod(0o644);run('--verify',str(stage),'/dev/pioneer-test',ok=False);target.chmod(0o600)
    target.unlink();target.symlink_to(stage/'small38');run('--verify',str(stage),'/dev/pioneer-test',ok=False)
    failed=run('--create','/dev/pioneer-test',ok=False,extra={'PIONEER_BFS_PROBE_TEST_FAIL':'flush'})
    failed_stage=Path(next(line[6:] for line in failed.stdout.splitlines() if line.startswith('STAGE ')))
    assert failed_stage.is_dir() and len(list(failed_stage.iterdir()))==4
    print('PASS fake-flush host probe creation, readback, invalid-device/path/content/mode/symlink refusals and preserved failed stage; NO DEVICE')
