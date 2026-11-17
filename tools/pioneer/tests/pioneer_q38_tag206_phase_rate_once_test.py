#!/usr/bin/env python3
"""Exercise the real once/restore control flow with a fake manager; no PCI IO."""

import os
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


RUNNER = Path(__file__).resolve().parents[1] / "pioneer_q38_tag206_phase_rate_once.sh"
source = RUNNER.read_text()
assert source.startswith("#!/bin/bash\n")
head, body = source.split('\n[ "$#" -eq 1 ]', 1)
body = '\n[ "$#" -eq 1 ]' + body

FAKES = r'''
stage=$PIONEER_FAKE_STAGE
installed=$stage/installed
candidate=$stage/test-candidate
stable=$stage/test-stable
admin=fake_admin
client=fake_client
stamp=$stage/manager
uname(){ printf '%s\n' Haiku; }
id(){ printf '%s\n' 0; }
paths(){ test -d "$stage" && test ! -L "$stage"; }
idle(){ ! test -e "$stage/not_idle"; }
health(){ true; }
exact(){
 test -f "$1" && test ! -L "$1" && {
 case "$2" in
 "$stableHash") test "$(< "$1")" = stable;;
 "$candidateHash") test "$(< "$1")" = candidate;;
 *) return 1;;
 esac; }
}
inputs(){
 paths && test ! -e "$stage/.trial-once" && exact "$installed" "$stableHash" && idle && metadata "$stableTag" 1 || fail preflight
}
sync(){ true; }
fake_admin(){
 local tag generation flags
 read -r tag generation flags < "$stamp"
 case "$1" in
 --status) printf 'P100A_RELOAD driver55 tag%s generation%s %s open_count1 guard1;CPU_METADATA_ONLY\n' "$tag" "$generation" "$flags"; return 0;;
 --quiesce)
  test "$tag" = "$2" && test "$flags" = 'quiesced0 cold0 active_owner0' || return 1
  printf '%s %s quiesced1 cold0 active_owner0\n' "$tag" "$generation" > "$stamp";;
 --wait)
  test "$flags" = 'quiesced1 cold0 active_owner0' && test "$3" = "$(printf '%016x' "$generation")" || return 1
  printf '%s %s quiesced0 cold0 active_owner0\n' "$2" "$((generation+1))" > "$stamp";;
 *) return 1;;
 esac
 printf '%s %s\n' "$1" "$2" >> "$stage/mutations"
}
fake_client(){
 test "$1" = --full-card-rate-32 && metadata "$candidateTag" 2 >/dev/null && exact "$installed" "$candidateHash" || return 9
 case "$PIONEER_FAKE_CASE" in
 owned) printf '%s 2 quiesced0 cold1 active_owner1\n' "$candidateTag" > "$stamp"; return 1;;
 fail) return 1;;
 bad-marker) return 0;;
 esac
 printf '%s\n' 'PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available1 output_tokens32;CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY'
}
'''


def main():
    subprocess.run(["bash", "-n", RUNNER], check=True)
    with TemporaryDirectory(prefix="pioneer-tag206-parent-") as tmp:
        for name in ("preflight", "stale", "success", "fail", "bad-marker", "owned"):
            stage = Path(tmp) / name
            stage.mkdir()
            for file, content in {"installed": "stable", "test-stable": "stable",
                                  "test-candidate": "candidate"}.items():
                (stage / file).write_text(content + "\n")
            generation = 4 if name == "stale" else 1
            (stage / "manager").write_text(f"2026102100010012 {generation} quiesced0 cold0 active_owner0\n")
            cmd = ["bash", "-c", head + FAKES + body, "mock-pioneer-runner",
                   "--preflight" if name == "preflight" else "--run"]
            env = {**os.environ, "PIONEER_FAKE_STAGE": str(stage), "PIONEER_FAKE_CASE": name}
            result = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
            assert result.returncode == (0 if name in ("success", "preflight") else 1), result
            if name in ("preflight", "stale"):
                assert not (stage / "mutations").exists()
                assert not (stage / ".trial-once").exists()
                continue
            mutations = (stage / "mutations").read_text().splitlines()
            assert mutations[:2] == ["--quiesce 2026102100010012", "--wait 2026111500010206"]
            if name == "owned":
                assert len(mutations) == 2 and (stage / "installed").read_text() == "candidate\n"
                assert "DO_NOT_RETRY_OR_RESET" in (stage / "state").read_text()
            else:
                assert mutations[2:] == ["--quiesce 2026111500010206", "--wait 2026102100010012"]
                assert (stage / "installed").read_text() == "stable\n"
                assert (stage / "state").read_text() == "STABLE041_GEN3_RESTORED\n"
            spent = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
            assert spent.returncode == 1 and (stage / "mutations").read_text().splitlines() == mutations
    print("PASS: preflight/stale generation, one launch, clean rollback, owned refusal, replay refusal; fake manager only")


if __name__ == "__main__":
    main()
