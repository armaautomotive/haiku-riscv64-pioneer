#!/usr/bin/env python3
"""Exercise the one-shot recovery procedure against a disposable fake host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'tools/pioneer/pioneer_qwen35_recovery_qualify.sh'
RESTORE = ROOT / 'tools/pioneer/pioneer_qwen35_qualify_restore_after_power_cycle.sh'
OLD_DRIVER = '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
OLD_GUARD = 'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'
NEW_GUARD = 'adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4'
ADMIN = 'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'
CANDIDATE = 'a0c75c115d9763be52fcb3b27ab47bb0ca58fd613e64de920ef2ec8544195ace'
CLIENT = '86cce1a7363ceaa7e6230016fbeaa03d383c91e0dc1689c68008a52c947d08fb'


def put(path, content, mode=0o755):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    path.chmod(mode)
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixture(root):
    home = root / 'boot/home'
    stage = home / 'pioneer-q35-recovery-qualify-20261123'
    stage.mkdir(parents=True, mode=0o700)
    trusted = home / 'pioneer-p100a-hot-reload-20261019-a'
    installed = home / 'config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe'
    guard = home / 'config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard'
    device = home / 'config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe'
    device.parent.mkdir(parents=True)
    device.symlink_to('../../bin/tenstorrent_pci_probe')
    stable_hash = put(trusted / 'tenstorrent_pci_probe-scale-reuse-qualification-041fef18', 'stable')
    assert put(installed, 'stable') == stable_hash
    old_guard_hash = put(guard, 'old guard')
    new_guard_hash = put(stage / 'new.guard', 'new guard')
    candidate_hash = put(stage / 'candidate', 'candidate')
    admin_hash = put(trusted / 'pioneer-p100a-reload', '''#!/usr/bin/env python3
import json, os, sys
p=os.environ['MOCK_STATE']
s=json.load(open(p))
if sys.argv[1]=='--status':
 print(f"P100A_RELOAD driver{s['driver']} tag{s['tag']} generation{s['generation']} quiesced{int(s['quiesced'])} cold{int(s['cold'])} active_owner{int(s['active'])} open_count1 guard1;CPU_METADATA_ONLY")
elif sys.argv[1]=='--quiesce':
 assert sys.argv[2]==s['tag'] and not s['quiesced'] and not s['cold'] and not s['active']
 s['quiesced']=True
elif sys.argv[1]=='--wait':
 assert s['quiesced'] and int(sys.argv[3],16)==s['generation']
 s['tag']=sys.argv[2];s['driver']=60 if s['tag']=='2026112300030001' else 55
 s['generation']+=1;s['quiesced']=False
else: raise SystemExit(2)
json.dump(s,open(p,'w'))
''')
    client_hash = put(stage / 'client', '''#!/usr/bin/env python3
import json, os, sys
assert sys.argv[1:] == ['--qualify-recovery']
p=os.environ['MOCK_STATE'];s=json.load(open(p));assert s['driver']==60
if os.environ.get('MOCK_FAIL')=='1':
 s['cold']=True;s['active']=True;json.dump(s,open(p,'w'));raise SystemExit(1)
s['generation']+=1;json.dump(s,open(p,'w'))
print('P100A_Q35_RECOVERY_QUALIFY reset_completed=1',file=sys.stderr)
''')
    for path, size in ((home/'pioneer-qwen35-4b-225/Qwen3.5-4B-Q4_K_M.gguf', 2740937888),
                       (home/'models/Qwen3.8-27B-Q4_K_M.gguf', 18973870528)):
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('wb') as stream:
            stream.truncate(size)
        path.chmod(0o600)
    bindir = root / 'bin'
    put(bindir/'uname', '#!/bin/sh\necho Haiku\n')
    put(bindir/'id', '#!/bin/sh\necho 0\n')
    put(bindir/'sync', '#!/bin/sh\nexit 0\n')
    put(bindir/'sysinfo', '#!/bin/sh\nprintf "64 CPUs running at 1 GHz\\nused/max 1 / 137034936320)\\n"\n')
    put(bindir/'ps', '#!/bin/sh\nprintf "   Id Team\\n    1 kernel_team\\n"\n')
    put(bindir/'stat', '''#!/usr/bin/env python3
import os, stat, sys
s=os.stat(sys.argv[3]);f=sys.argv[2]
values={'%u:%h':f'{s.st_uid}:{s.st_nlink}', '%u:%a':f'{s.st_uid}:{stat.S_IMODE(s.st_mode):o}',
 '%u:%h:%a:%s':f'{s.st_uid}:{s.st_nlink}:{stat.S_IMODE(s.st_mode):o}:{s.st_size}', '%d':str(s.st_dev)}
print(values[f])
''')
    put(bindir/'sha256sum', '''#!/usr/bin/env python3
import hashlib,sys
p=sys.argv[1];print(hashlib.sha256(open(p,'rb').read()).hexdigest(),p)
''')
    state = root/'driver-state.json'
    state.write_text(json.dumps(dict(driver=55, tag='2026102100010012', generation=1,
                                     quiesced=False, cold=False, active=False)))
    script = SCRIPT.read_text().replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
    script = script.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
    for old, new in ((OLD_DRIVER, stable_hash), (OLD_GUARD, old_guard_hash), (NEW_GUARD, new_guard_hash),
                     (ADMIN, admin_hash), (CANDIDATE, candidate_hash), (CLIENT, client_hash)):
        assert old in script
        script = script.replace(old, new)
    wrapper = root/'qualify.sh'
    put(wrapper, script)
    env = dict(os.environ, PATH=str(bindir)+':'+os.environ['PATH'], MOCK_STATE=str(state))
    return home, stage, installed, guard, wrapper, env


def run(wrapper, env, mode):
    return subprocess.run(['bash', str(wrapper), mode, '1'], env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)


def main():
    with tempfile.TemporaryDirectory() as temp:
        home, stage, installed, guard, wrapper, env = fixture(Path(temp))
        assert run(wrapper, env, '--stage-guard').returncode == 0
        assert guard.read_text() == 'new guard' and (stage/'guard.previous').read_text() == 'old guard'
        assert run(wrapper, env, '--run').returncode == 0
        assert installed.read_text() == 'stable' and guard.read_text() == 'new guard'
        assert (stage/'state').read_text().strip() == 'RECOVERY_QUALIFIED_STABLE_RESTORED'
        assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
        assert run(wrapper, env, '--run').returncode != 0
    with tempfile.TemporaryDirectory() as temp:
        home, stage, installed, guard, wrapper, env = fixture(Path(temp))
        assert run(wrapper, env, '--stage-guard').returncode == 0
        failed = run(wrapper, dict(env, MOCK_FAIL='1'), '--run')
        assert failed.returncode != 0 and installed.read_text() == 'candidate'
        assert (stage/'state').read_text().strip() == 'QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
        assert (stage/'restore.incoming').read_text() == 'stable'
        assert (home/'.pioneer-q35-dataflow-exclusive').is_dir()
        # Simulate a fresh kernel after a full power cycle; this is not proof
        # that the hardware was reset or that in-place recovery worked.
        state = Path(env['MOCK_STATE'])
        state.write_text(json.dumps(dict(driver=60, tag='2026112300030001', generation=1,
                                         quiesced=False, cold=False, active=False)))
        restore = RESTORE.read_text().replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        restore = restore.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for old, path in ((OLD_DRIVER, stage/'restore.incoming'), (NEW_GUARD, guard),
                          (ADMIN, home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'),
                          (CANDIDATE, stage/'candidate')):
            restore = restore.replace(old, hashlib.sha256(path.read_bytes()).hexdigest())
        restore_path = Path(temp)/'restore.sh'
        put(restore_path, restore)
        result = subprocess.run(['bash', str(restore_path), str(stage)], env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert installed.read_text() == 'stable' and not (home/'.pioneer-q35-dataflow-exclusive').exists()
    with tempfile.TemporaryDirectory() as temp:
        home, stage, installed, guard, wrapper, env = fixture(Path(temp))
        (stage/'candidate').write_text('changed')
        assert run(wrapper, env, '--stage-guard').returncode != 0
        assert guard.read_text() == 'old guard' and installed.read_text() == 'stable'
        assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
    print('PASS one-shot qualification stages guard, restores stable driver and retains failed state; SIMULATED HOST ONLY')


if __name__ == '__main__':
    main()
