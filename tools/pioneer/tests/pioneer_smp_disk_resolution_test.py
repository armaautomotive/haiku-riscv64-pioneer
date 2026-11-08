#!/usr/bin/env python3
"""Exercise the production disk resolver with synthetic identities only."""
import os
from pathlib import Path
import subprocess
import tempfile

script = Path(__file__).resolve().parents[1] / 'pioneer_linux_boot_haiku_smp.sh'
text = script.read_text()
start = text.index('resolve_recorded_disks()\n{')
end = text.index('\n}\nresolve_recorded_disks', start) + 2
resolver = text[start:end]
mocks = r'''
fail() { printf 'refused: %s\n' "$1" >&2; exit 1; }
readlink() {
 case "$2" in
  *S14CNSAD300075H-part1) echo "${SAMSUNG}1";;
  *S14CNSAD300075H) echo "$SAMSUNG";;
  *E201610050040028-part2) echo "$BUILD_PART";;
  *E201610050040028) echo "$BUILD";;
  *) return 1;;
 esac
}
lsblk() {
 case "$4" in "$SAMSUNG") echo "$SAM_SERIAL";; "$BUILD") echo E201610050040028;; *) return 1;; esac
}
blkid() {
 case "$5" in "${SAMSUNG}1") echo "$BFS_UUID";; "$BUILD_PART") echo d7acf635-9bfa-43b5-abf9-a35bf5abfd82;; *) return 1;; esac
}
findmnt() {
 case "$1" in -n) echo "$BUILD_MOUNT";; -rn) [ "$MOUNTED" = 0 ] || echo mounted;; *) return 1;; esac
}
blockdev() { echo "$SAM_SIZE"; }
cat() { echo "$SAM_START"; }
'''


def run(**changes):
    env = dict(os.environ, SAMSUNG='/dev/sda', BUILD='/dev/sdb', BUILD_PART='/dev/sdb2',
               SAM_SERIAL='S14CNSAD300075H', BFS_UUID='020147de08752551',
               BUILD_MOUNT='/dev/sdb2', MOUNTED='0', SAM_SIZE='120034123776', SAM_START='64')
    env.update(changes)
    return subprocess.run(['sh', '-c', mocks + resolver + '\nresolve_recorded_disks\nprintf "%s %s\\n" "$samsung_disk" "$build_partition"\n'],
                          env=env, capture_output=True, text=True)


assert run().returncode == 0
shifted = run(SAMSUNG='/dev/sdb', BUILD='/dev/sdc', BUILD_PART='/dev/sdc2', BUILD_MOUNT='/dev/sdc2')
assert shifted.returncode == 0 and shifted.stdout.strip() == '/dev/sdb /dev/sdc2'
for invalid in [dict(SAM_SERIAL='B0196SUD'), dict(BFS_UUID='wrong'), dict(SAM_SIZE='1'),
                dict(SAM_START='65'), dict(MOUNTED='1'), dict(BUILD_MOUNT='/dev/sdc2'),
                dict(BUILD_PART='/dev/sda2'), dict(SAMSUNG='/dev/nvme0n1'),
                dict(SAMSUNG='/dev/sdb', BUILD='/dev/sdb')]:
    result = run(**invalid)
    assert result.returncode != 0, (invalid, result.stdout, result.stderr)
assert '[ "$samsung_disk" = /dev/sda ] || fail' in text
assert 'dd if="$samsung_disk"' in text
assert 'mount -t befs -o ro,nosuid,nodev,noexec "$samsung_partition"' in text
manifest_start = text.index('root_manifest()\n{')
manifest_end = text.index('\n}\n', manifest_start) + 2
manifest_function = text[manifest_start:manifest_end]
manifest_mock = r'''
samsung_disk=/dev/sdb
dd() { [ "$1" = if=/dev/sdb ] || return 1; printf 'sector\n'; }
od() {
 read -r data || return 1
 [ "$data" = sector ] || return 1
 n=0
 while [ "$n" -lt "$WORDS" ]; do echo 4294967295; n=$((n + 1)); done
}
'''
good = subprocess.run(['sh', '-c', manifest_mock + manifest_function + '\nroot_manifest\n'],
                      env=dict(os.environ, WORDS='128'), capture_output=True, text=True)
assert good.returncode == 0 and len(good.stdout.split()) == 20
short = subprocess.run(['sh', '-c', manifest_mock + manifest_function + '\nroot_manifest\n'],
                       env=dict(os.environ, WORDS='127'), capture_output=True, text=True)
assert short.returncode != 0
rtl_start = text.index('hash_is_recorded_rtl()\n{')
rtl_end = text.index('\n}\n', rtl_start) + 2
rtl_function = text[rtl_start:rtl_end]
with tempfile.TemporaryDirectory(prefix='pioneer-rtl-pin-test-') as directory:
    candidate = Path(directory) / 'rtl'
    candidate.write_bytes(b'synthetic')
    link = Path(directory) / 'link'
    link.symlink_to(candidate)
    shell = ('fail() { exit 1; }\nsha256sum() { echo "$DIGEST  $1"; }\n'
             + rtl_function + '\nhash_is_recorded_rtl "$1"\n')
    for digest, good in [('fe6273c985de229e0c6d8fba9dcc27d7978cb9ab2b419670058ceb0198a13c00', True),
                         ('0b83ceed0071f3dab3f9b175355bd83fe7364176676264ae66d751ddba2b7c9a', True),
                         ('0' * 64, False)]:
        result = subprocess.run(['sh', '-c', shell, 'rtl-pin-test', str(candidate)],
                                env=dict(os.environ, DIGEST=digest), capture_output=True)
        assert (result.returncode == 0) == good
    result = subprocess.run(['sh', '-c', shell, 'rtl-pin-test', str(link)],
                            env=dict(os.environ, DIGEST='0b83ceed0071f3dab3f9b175355bd83fe7364176676264ae66d751ddba2b7c9a'), capture_output=True)
    assert result.returncode != 0
print('PASS: original and reordered SATA identities; wrong target and mounted-source refusals; no disk access')
