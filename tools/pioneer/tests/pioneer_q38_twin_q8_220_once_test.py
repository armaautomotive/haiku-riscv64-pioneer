#!/usr/bin/env python3
"""Run the physical runner's actual transition/receipt logic with fake Haiku I/O."""
import os
from hashlib import sha256
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory

RUNNER = Path(__file__).resolve().parents[1] / "pioneer_q38_twin_q8_220_once.sh"
sys.path.insert(0, str(RUNNER.parent))
from build_qwen38_twin_q8_221_trial import bind
GATE = '\n[ "$(uname -s)" = Haiku ]'
FAKES = r'''
stage=$TEST_STAGE
lock=$stage/shared-lock
installed=$stage/installed
device=$stage/dev-link
stable=$stage/stable
candidate=$stage/candidate
client=fake_client
guard=$stage/guard
admin=fake_admin
uname(){ printf '%s\n' Haiku; }
id(){ printf '%s\n' 0; }
stat(){ case "$2" in '%d') echo 17;; '%u:%a') echo 0:700;; esac; }
test(){
 if [[ "$*" = '-x fake_client' || "$*" = '-f /boot/system/lib/libstdc++.so.6' ]]; then return 0; fi
 builtin test "$@"
}
ps(){
 printf 'Id Team\n1 kernel\n'
 if [ "$TEST_CASE" = live ]; then printf '98 pioneer-q38-twin-q8-compare\n'; fi
 if [ "$TEST_CASE" = parent-live ]; then printf '%s /boot/system/bin/bash\n' "$(< "$stage/launch.pid")"; fi
}
sysinfo(){ printf '64 (Unknown), revision 0000 running at 1000MHz\n123 bytes free (used/max 134 / 137034936320)\n'; }
exact(){
 case "$1" in
 fake_admin|fake_client) return 0;;
 esac
 test -f "$1" && test ! -L "$1" || return 1
 case "$2" in
 "$stableHash") test "$(< "$1")" = stable;;
 "$candidateHash") test "$(< "$1")" = candidate;;
 "$manifestHash") test "$(< "$1")" = manifest;;
 *) test "$(< "$1")" = guard;;
 esac
}
sha256sum(){
 if command -v sha256sum >/dev/null && [ "$(type -P sha256sum)" ]; then
  command sha256sum "$@"
 else
  shasum -a 256 "$@"
 fi
}
sync(){ :; }
fake_admin(){
 local tag generation flags
 read -r tag generation flags < "$stage/manager"
 case "$1" in
 --status)
  printf '%s\n' status >> "$stage/status-calls"
  printf 'P100A_RELOAD driver55 tag%s generation%s %s open_count1 guard1;CPU_METADATA_ONLY\n' "$tag" "$generation" "$flags"
  return 0;;
 --quiesce)
  [[ "$TEST_CASE" != initial-failure || "$tag" != "$stableTag" ]] || return 1
  [[ "$TEST_CASE" != restore-failure || "$tag" != "$candidateTag" ]] || return 1
  test "$tag" = "$2" && test "$flags" = 'quiesced0 cold0 active_owner0' || return 1
  printf '%s %s quiesced1 cold0 active_owner0\n' "$tag" "$generation" > "$stage/manager";;
 --wait)
  test "$flags" = 'quiesced1 cold0 active_owner0' && test "$3" = "$(printf '%016x' "$generation")" || return 1
  printf '%s %s quiesced0 cold0 active_owner0\n' "$2" "$((generation+1))" > "$stage/manager";;
 *) return 1;;
 esac
 printf '%s %s\n' "$1" "$2" >> "$stage/exchanges"
}
fake_client(){
 test "$1" = --compare && test "$2" = "$((TEST_START+1))" && exact "$installed" "$candidateHash" || return 9
 case "$TEST_CASE" in
 cold)
  printf '%s %s quiesced0 cold1 active_owner1\n' "$candidateTag" "$((TEST_START+1))" > "$stage/manager"; return 1;;
 clean-failure) return 1;;
 unexpected-stderr) echo noise >&2;;
 esac
 local v arm d
 for v in 0 1; do
  if [ "$v" = 0 ]; then
   d=0e94ceba949d9266e0383ca43b96c73da4bcaf0785fec9d45b4570f73e021254
  else
   d=42f5a264959edb0b327fc337a227ae2f4bcb55bc74a637cbde09087b057ebfd2
  fi
  for arm in 1 2; do
   if [ "$TEST_CASE" = truncated ] && [ "$v:$arm" = 1:2 ]; then break; fi
   if [ "$TEST_CASE" = wrong-hash ] && [ "$v:$arm" = 0:2 ]; then d=incorrect; fi
   if [ "$TEST_CASE" = wrong-arm ] && [ "$v:$arm" = 0:2 ]; then arm=1; fi
   printf 'Q8_PAIR_CLOSED arm=%s variant=%s pages=68 polls=20 matrix_wait_us=310 output_sha=%s\n' "$arm" "$v" "$d"
  done
 done
 printf '%s\n' 'PASS_Q8_PAIR_FOUR_CLOSED_MATCHING_PINNED_ARMS;NO_TOKENS_PER_SECOND'
}
'''


def exercise(text, tag, start):
    subprocess.run(["bash", "-n"], input=text, text=True, check=True)
    head, body = text.split(GATE, 1)
    grep = shutil.which("grep")
    assert grep
    source = head.replace("/bin/grep", grep) + FAKES + GATE + body
    stable, candidate = "2026102100010012", "2026111700010" + str(tag)
    before = ("preflight", "wrong-manifest", "stale", "lock", "dangling-lock", "live")
    unconfirmed = ("cold", "initial-failure", "restore-failure")
    negative_receipts = ("clean-failure", "wrong-arm", "wrong-hash", "truncated", "unexpected-stderr")
    with TemporaryDirectory(prefix="pioneer-tag220-fakes-") as tmp:
        for name in (*before, "success", *negative_receipts, *unconfirmed):
            stage = Path(tmp) / name
            stage.mkdir()
            for file, content in {"installed": "stable", "stable": "stable", "candidate": "candidate",
                                  "manifest.json": "wrong" if name == "wrong-manifest" else "manifest",
                                  "guard": "guard"}.items():
                (stage / file).write_text(content + "\n")
            (stage / "run-once.sh").write_text(text)
            (stage / "dev-link").symlink_to("../../bin/tenstorrent_pci_probe")
            lock = stage / "shared-lock"
            if name == "lock":
                lock.mkdir()
            if name == "dangling-lock":
                lock.symlink_to(stage / "absent")
            initial = start + (1 if name == "stale" else 0)
            (stage / "manager").write_text(f"{stable} {initial} quiesced0 cold0 active_owner0\n")

            def run(*args, case=name):
                env = {**os.environ, "TEST_STAGE": str(stage), "TEST_CASE": case, "TEST_START": str(start)}
                return subprocess.run(["bash", "-c", source, "mock-tag220", *args],
                                      env=env, text=True, capture_output=True, timeout=10)

            result = run("--preflight" if name == "preflight" else "--run")
            assert result.returncode == (0 if name in ("success", "preflight") else 1), (
                name, result.returncode, result.stdout, result.stderr)
            exchanges = stage / "exchanges"
            if name in before:
                assert not exchanges.exists() and not (stage / ".trial-once").exists(), name
                continue
            (stage / "parent.stdout").write_text(result.stdout)
            (stage / "parent.stderr").write_text(result.stderr)
            parent = (stage / "launch.pid").read_text().strip()
            raw_events = exchanges.read_bytes() if exchanges.exists() else b""
            events = raw_events.decode().splitlines()
            assert run("--run").returncode == 1
            assert (exchanges.read_bytes() if exchanges.exists() else b"") == raw_events
            statuses = stage / "status-calls"
            count = len(statuses.read_bytes())
            assert run("--postflight", "0").returncode == 1
            assert run("--postflight", str(int(parent) + 1)).returncode == 1
            assert run("--postflight", parent, case="parent-live").returncode == 1
            assert len(statuses.read_bytes()) == count, name
            post = run("--postflight", parent)
            assert post.returncode == 0, (name, post.stdout, post.stderr)
            if name in unconfirmed:
                assert events == ([] if name == "initial-failure" else [f"--quiesce {stable}", f"--wait {candidate}"])
                assert lock.is_dir() and "LOCK_RETAINED_NO_DEVICE_OPEN" in post.stdout
                assert len(statuses.read_bytes()) == count
                assert (stage / "installed").read_text() == ("stable\n" if name == "initial-failure" else "candidate\n")
            else:
                assert events == [f"--quiesce {stable}", f"--wait {candidate}",
                                  f"--quiesce {candidate}", f"--wait {stable}"]
                assert not lock.exists() and (stage / "installed").read_text() == "stable\n"
                marker = ("VERIFIED_FOUR_ARMS_AND_STABLE_RESTORED" if name == "success"
                          else "CLIENT_FAILED_BUT_STABLE_RESTORED")
                assert marker in post.stdout, (name, post.stdout, post.stderr)
    print(f"PASS 15 tag{tag} gen{start} fake-manager cases; no live or uncertain postflight device open")


def main():
    for bad in ("0", "01", " -1", "1\n", "1; exit", "1000000"):
        try:
            bind(bad)
        except ValueError:
            pass
        else:
            raise AssertionError(bad)
    assert sha256(bind("2").encode("ascii")).hexdigest() == (
        "d6745833830e195886f60b4b7f90d351f74679bab7497e13db8783ad6bfbb0e9")
    try:
        bind("2", "220")
    except ValueError:
        pass
    else:
        raise AssertionError("spent tag220 cannot be rebound")
    exercise(RUNNER.read_text(), 220, 4)
    exercise(bind("2"), 221, 2)
    exercise(bind("15"), 221, 15)
    exercise(bind("2", "222"), 222, 2)


if __name__ == "__main__":
    main()
