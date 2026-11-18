#!/usr/bin/env python3
"""Bind the verified boot correction to one fresh stable generation. Build only."""
from hashlib import sha256
from pathlib import Path
import re
import sys

TEMPLATE = Path(__file__).with_name("pioneer_q38_twin_q8_220_once.sh")
TEMPLATE_SHA = "043d4f2f848a47431a12bbec52bda72609008015d86ecbf085ccf23108c244ce"
PACKAGES = {
    "221": ("39c234a37d77d369010166996dc56bae173283e91f67d0888994d31d047fea32",
            "12102334568434f495d537bd5a0cd43f770a0a5abd0093fe43b361c30152b7ae",
            "00d6611903d73c0cb364cef83acf98b989fef36506dab2c47dde856ce9295a1a"),
    "222": ("ee6a90e9c87086efc61d006a3d2621288ba5d70379358059cf0db18eb245bc14",
            "aeb25b18c36ef40a60ca0b82076b00c23fbcd51d7541f3e57296e884eba71c1d",
            "20faba0731cddfe7db597a60a6f0b9583bf4c7126871883c1cedda40cf9962bd"),
    "223": ("fef066d1f178e5184a376c63b56fcadfd963a5be3f5afe5de51b1187bc130d00",
            "1d939f19c2ec5042a2ada346b8de6f52a815cba4f947ed9f817bce733cc1964a",
            "d7bc3d0ac937496d37a7fbc8b6d3f0566333cb8a894585499226dff3104761e2"),
}


def bind(stable_generation, tag="221"):
    if not re.fullmatch(r"[1-9][0-9]{0,5}", stable_generation):
        raise ValueError("supply an observed positive stable generation")
    if tag not in PACKAGES:
        raise ValueError("unknown diagnostic package")
    driver, client, manifest = PACKAGES[tag]
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
        ("twin-q8-220", f"twin-q8-{tag}", 2),
        ("2026111700010220", f"2026111700010{tag}", 1),
        ("8bfd096f35df5b1dd2c96f368e774cd1631b714cff256b7c46e345a8de6bce6a", driver, 1),
        ("4140e029a0d21207580cca9ea15a9a01465b26c293f937d79ebe55d5742ccbda", client, 1),
        ("6ba3e08c4d88ac2950dd500433828c121b20037bd8f3f103318f4132de947e32", manifest, 1),
    ):
        assert text.count(old) == count, old
        text = text.replace(old, new)
    return text.replace("tag220", f"tag{tag}").replace("TAG220", f"TAG{tag}")


def main():
    tag = {"--tag222": "222", "--tag223": "223"}.get(sys.argv[3]) if len(sys.argv) == 4 else "221"
    if not __debug__ or len(sys.argv) not in (3, 4) or tag is None:
        raise SystemExit("use OBSERVED_STABLE_GENERATION NEW_LOCAL_SCRIPT [--tag222|--tag223]")
    data = bind(sys.argv[1], tag).encode("ascii")
    with Path(sys.argv[2]).open("xb") as out:
        out.write(data)
    print(sha256(data).hexdigest(), sys.argv[2])


if __name__ == "__main__":
    main()
