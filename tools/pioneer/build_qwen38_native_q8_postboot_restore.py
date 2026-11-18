#!/usr/bin/env python3
"""Bind the existing post-boot restore policy to the spent tag217. No board I/O."""

from hashlib import sha256
from pathlib import Path
import subprocess
import sys


TEMPLATE = Path(__file__).with_name("pioneer_q8t4_postboot_restore_once.sh")
TEMPLATE_SHA = "26203aff1d70bd539f4e4b7cea2d781a4753dc796df27d57e6de512b92318dde"
ARCHIVE_SHA = {
    "rate.stdout": "dd95cdb58436a055bcbb3a42caa39f45b2f99d78f5dc277085741bde415e741d",
    "rate.stderr": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "rate.exit": "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865",
    "parent.stdout": "d8eb4a4ed0271fdf4140859f8350f5885f0bba013c73909a21fb8a1a162a1841",
    "parent.stderr": "6954cf2edcbffc17b53bafd7bd6a68d7ad21eef87a96d150ba05fa0044d8db6b",
    "parent.exit": "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865",
    "state": "86aadc66b39017b2b77524edd86c8ca60a604bfa63cf2bd03d985d27771611b5",
    "run-once.sh": "23333c8b67dec3585daf0ca4fb56c586c7df6f75ec6a608019d3b552cfba6084",
    "launch.pid": "21b178d6790f4bb5dcde303c9cde62c5201cfb712e6ec102a109254538b9ed5a",
}
# These are the independently repeated observations after the OFF/ON
# recovery. They are never accepted as evidence of the failed computation.
POSTBOOT_SHA = {
    "state": "d37d3860b2250969ec79c5e2706b89e19792af3d0e213b29c983238845720890",
    "rate.exit": "70b8c6f1bfd21d40614974d89b8fe17c9e8628c6479a7a6c6f27fd769838a998",
    "parent.exit": "8fc43d3084cd8032a3c7f3a0408031936c22bdeffbbcd81bb16051b5d4420ef7",
    "rate.stdout": "26e5b201a122196a1d85fa4adc5c8a6916b9cc1e354ebd60599d3b6629afc288",
    "parent.stdout": "02a3cf10f91d09edcd7c523faff0f7964287f1ba64de9baa4227ff75cdf45803",
    "parent.stderr": "eac86c1f193a07cb3e33c6db1bcb6e3599262fa5f1924aaf8e086bf11eaf5c23",
}


def bind(template, archive=None):
    if sha256(template).hexdigest() != TEMPLATE_SHA:
        raise ValueError("restore policy changed; review before rebinding")
    text = template.decode("ascii")
    for old, new, count in (
        ("Q8T4_POSTBOOT", "NATIVE_Q8_POSTBOOT", 2),
        ("pioneer-q38-two-token-q4-215", "pioneer-q38-native-q8-wire-217", 1),
        ("pioneer-q8t4-postboot-restore-215", "pioneer-q8-postboot-restore-217", 1),
        ("d73dbf803bd05087bd4afb554b841032a8a383ac5495bc49193b4e8870670cd2",
         "3303735cb89298c7b7b4be70b0133965923375cfce135f456fbc8ff4757ae749", 1),
        ("2026111700010215", "2026111700010217", 1),
        ("exact \"$failed/compare.exit\" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || fail failed_exit",
         "exact \"$failed/rate.exit\" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || fail failed_rate_exit\n"
         "exact \"$failed/parent.exit\" 4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865 || fail failed_parent_exit\n"
         "exact \"$failed/launch.pid\" 21b178d6790f4bb5dcde303c9cde62c5201cfb712e6ec102a109254538b9ed5a || fail failed_parent_team\n"
         "exact \"$failed/rate.stdout\" dd95cdb58436a055bcbb3a42caa39f45b2f99d78f5dc277085741bde415e741d || fail failed_rate_output\n"
         "exact \"$failed/parent.stderr\" 6954cf2edcbffc17b53bafd7bd6a68d7ad21eef87a96d150ba05fa0044d8db6b || fail failed_parent_error\n"
         "test -z \"$(ls -A \"$lock\")\" || fail lock_not_empty", 1),
        ("de652e39c9de1e7b3814eb7aa4820cd7d340c90835a6e72e80516efba0cc1ab4",
         "23333c8b67dec3585daf0ca4fb56c586c7df6f75ec6a608019d3b552cfba6084", 1),
    ):
        if text.count(old) != count:
            raise ValueError("unexpected restore policy replacement")
        text = text.replace(old, new)
    if archive is not None:
        # Local, pre-power evidence; refuse arbitrary post-power hashes or
        # rewriting any of the recorded files. The complete driver and guard
        # identities and the fresh-boot metadata gate below remain unchanged.
        if not archive.is_absolute() or not archive.is_dir() or archive.is_symlink():
            raise ValueError("provide the absolute verified pre-power archive")
        for name, digest in ARCHIVE_SHA.items():
            path = archive / name
            if path.is_symlink() or not path.is_file() or sha256(path.read_bytes()).hexdigest() != digest:
                raise ValueError(f"pre-power archive mismatch: {name}")
        before, count = rebind_failure_logs(text)
        if count != 5:
            raise ValueError("postboot fingerprints did not replace all existing guards")
        needle = 'test -z "$(ls -A "$lock")" || fail lock_not_empty'
        if before.count(needle) != 1:
            raise ValueError("exclusive lock changed")
        for name in ("parent.stdout", "rate.stderr"):
            before = before.replace(needle, f'exact "$failed/{name}" '
                                    f'{POSTBOOT_SHA.get(name, ARCHIVE_SHA[name])} || fail identified_log\n{needle}', 1)
        text = before
    return text.encode("ascii")


def rebind_failure_logs(text):
    count = 0
    for name, digest in POSTBOOT_SHA.items():
        old = f'exact "$failed/{name}" {ARCHIVE_SHA[name]} '
        if name == "parent.stdout":
            continue  # not present in the original pre-power binding
        if text.count(old) != 1:
            raise ValueError(f"expected one failed file check: {name}")
        text = text.replace(old, f'exact "$failed/{name}" {digest} ', 1)
        count += 1
    return text, count


if __name__ == "__main__":
    if (len(sys.argv) not in (2, 4) or not Path(sys.argv[1]).is_absolute()
            or (len(sys.argv) == 4 and sys.argv[2] != "--verified-pre-power-archive")):
        raise SystemExit("use ABSOLUTE_NEW_RESTORE_SCRIPT [--verified-pre-power-archive ARCHIVE]")
    result = bind(TEMPLATE.read_bytes(), None if len(sys.argv) == 2 else Path(sys.argv[3]))
    subprocess.run(["/bin/bash", "-n"], input=result, check=True)
    path = Path(sys.argv[1])
    with path.open("xb") as destination:
        destination.write(result)
    print(sha256(result).hexdigest(), path)
