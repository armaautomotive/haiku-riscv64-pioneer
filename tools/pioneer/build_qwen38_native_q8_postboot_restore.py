#!/usr/bin/env python3
"""Bind the existing post-boot restore policy to the spent tag217. No board I/O."""

from hashlib import sha256
from pathlib import Path
import subprocess
import sys


TEMPLATE = Path(__file__).with_name("pioneer_q8t4_postboot_restore_once.sh")
TEMPLATE_SHA = "26203aff1d70bd539f4e4b7cea2d781a4753dc796df27d57e6de512b92318dde"


def bind(template):
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
    return text.encode("ascii")


if __name__ == "__main__":
    if len(sys.argv) != 2 or not Path(sys.argv[1]).is_absolute():
        raise SystemExit("use ABSOLUTE_NEW_RESTORE_SCRIPT")
    result = bind(TEMPLATE.read_bytes())
    subprocess.run(["/bin/bash", "-n"], input=result, check=True)
    path = Path(sys.argv[1])
    with path.open("xb") as destination:
        destination.write(result)
    print(sha256(result).hexdigest(), path)
