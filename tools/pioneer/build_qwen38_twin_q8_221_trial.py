#!/usr/bin/env python3
"""Bind the verified boot correction to one fresh stable generation. Build only."""
from hashlib import sha256
from pathlib import Path
import re
import sys

TEMPLATE = Path(__file__).with_name("pioneer_q38_twin_q8_220_once.sh")
TEMPLATE_SHA = "043d4f2f848a47431a12bbec52bda72609008015d86ecbf085ccf23108c244ce"


def bind(stable_generation):
    if not re.fullmatch(r"[1-9][0-9]{0,5}", stable_generation):
        raise ValueError("supply an observed positive stable generation")
    start = int(stable_generation)
    data = TEMPLATE.read_bytes()
    if sha256(data).hexdigest() != TEMPLATE_SHA:
        raise ValueError("unexpected tag220 template; refuse rebinding")
    text = data.decode("ascii")
    text, count = re.subn(r'(metadata "\$(?:stable|candidate)Tag" )([456])\b',
                          lambda m: m[1] + str(start + int(m[2]) - 4), text)
    assert count == 6
    text, count = re.subn(r'(--wait "\$(?:stable|candidate)Tag" )000000000000000([45])\b',
                          lambda m: m[1] + f"{start + int(m[2]) - 4:016x}", text)
    assert count == 2
    text, count = re.subn(r'("\$client" --compare )5\b', lambda m: m[1] + str(start + 1), text)
    assert count == 1
    for old, new, count in (
        ("generation is 4.", f"generation is {start}.", 1),
        ("STABLE041_GEN6_RESTORED", f"STABLE041_GEN{start+2}_RESTORED", 2),
        ("twin-q8-220", "twin-q8-221", 2),
        ("2026111700010220", "2026111700010221", 1),
        ("8bfd096f35df5b1dd2c96f368e774cd1631b714cff256b7c46e345a8de6bce6a",
         "39c234a37d77d369010166996dc56bae173283e91f67d0888994d31d047fea32", 1),
        ("4140e029a0d21207580cca9ea15a9a01465b26c293f937d79ebe55d5742ccbda",
         "12102334568434f495d537bd5a0cd43f770a0a5abd0093fe43b361c30152b7ae", 1),
        ("6ba3e08c4d88ac2950dd500433828c121b20037bd8f3f103318f4132de947e32",
         "00d6611903d73c0cb364cef83acf98b989fef36506dab2c47dde856ce9295a1a", 1),
    ):
        assert text.count(old) == count, old
        text = text.replace(old, new)
    return text.replace("tag220", "tag221").replace("TAG220", "TAG221")


def main():
    if not __debug__ or len(sys.argv) != 3:
        raise SystemExit("use OBSERVED_STABLE_GENERATION NEW_LOCAL_SCRIPT")
    data = bind(sys.argv[1]).encode("ascii")
    with Path(sys.argv[2]).open("xb") as out:
        out.write(data)
    print(sha256(data).hexdigest(), sys.argv[2])


if __name__ == "__main__":
    main()
