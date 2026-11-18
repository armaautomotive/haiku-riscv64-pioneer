#!/usr/bin/env python3
"""Bind the existing guarded restore to this spent trial and its archived evidence."""
from hashlib import sha256
from pathlib import Path
import re
import sys

TEMPLATE = Path(__file__).with_name("pioneer_q38_twin_q8_220_recover.sh")
TEMPLATE_SHA = "9dd5b0782f7a1b84cb3d0090d868ce30232ca26b0fab66c7abfedeee17484bff"
PRE_POWER = {
    "state": "86aadc66b39017b2b77524edd86c8ca60a604bfa63cf2bd03d985d27771611b5",
    "launch.pid": "04ac70e5b43f47d205e6483b65209eb41e2e951aa57bf17e5b466112bbed9def",
    "run-once.sh": "d6745833830e195886f60b4b7f90d351f74679bab7497e13db8783ad6bfbb0e9",
    "compare.stderr": "5c8fc7168445982df3ef3579a32d28979ebdaf613d788c06dca850be88909572",
    "compare.exit": "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865",
    "parent.exit": "4355a46b19d348dc2f57c046f8ef63d4538ebb936000f3c9ee954a27460dd865",
    "parent.stdout": "728cf63429467396c41f6afa9d82351ea9e45311b9bdd5fcc00fc2c5e147af51",
    "parent.stderr": "c537be6e5e184e0bbfc5d8891123fa6b5d5f8e0521a5a5ef224bd8b18911ed67",
    "compare.stdout": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
}
MANIFEST = "00d6611903d73c0cb364cef83acf98b989fef36506dab2c47dde856ce9295a1a"
DRIVER = "39c234a37d77d369010166996dc56bae173283e91f67d0888994d31d047fea32"
NEXT_POWER = dict(PRE_POWER, **{
    "launch.pid": "8b43e07955cc3a9e17d74b960abf019bca14a764c87faa83fca03c00020683ee",
    "run-once.sh": "41c2072ac48f533f53e6119a6551caeb51e2b4be0b71cb2ee26ef27dfb4858c2",
    "compare.stderr": "f6d63ee73c257fb2970200eb8c1047a317d198d431871d4b65aee52fa869037a",
    "parent.stdout": "bd45665665ce1101c4110fdb1c40ac929350615d9397654ab5c95a5dfbe4b074",
    "parent.stderr": "aedd486c8c0f87c1437333874ff9ae1e3662a0827066340bbe3091049b485579",
})
PACKAGES = {"221": (PRE_POWER, MANIFEST, DRIVER),
            "222": (NEXT_POWER,
                    "20faba0731cddfe7db597a60a6f0b9583bf4c7126871883c1cedda40cf9962bd",
                    "ee6a90e9c87086efc61d006a3d2621288ba5d70379358059cf0db18eb245bc14")}


def pin(path):
    assert path.is_file() and not path.is_symlink() and path.stat().st_size < 65536, path
    return sha256(path.read_bytes()).hexdigest()


def bind(pre_power, post_power=None, tag="221"):
    archived, manifest, driver = PACKAGES[tag]
    data = TEMPLATE.read_bytes()
    assert sha256(data).hexdigest() == TEMPLATE_SHA
    for name, digest in archived.items():
        assert pin(pre_power / name) == digest, name
    bound = dict(archived, **{"manifest.json": manifest})
    text = data.decode("ascii")
    for name in ("state", "compare.stderr", "compare.exit", "parent.exit", "parent.stdout", "parent.stderr"):
        if post_power is not None and (post_power / name).exists():
            bound[name] = pin(post_power / name)
    for name, digest in bound.items():
        if name == "compare.stdout":
            continue
        expression = rf'(exact "\$stage/{re.escape(name)}" )[0-9a-f]{{64}}'
        text, count = re.subn(expression, lambda m: m[1] + digest, text)
        assert count == 1, name
    marker = 'exact "$stage/stable041.backup" "$stableHash"'
    assert text.count(marker) == 1
    text = text.replace(marker, f'exact "$stage/compare.stdout" {bound["compare.stdout"]} &&\n'+marker)
    text = text.replace('8bfd096f35df5b1dd2c96f368e774cd1631b714cff256b7c46e345a8de6bce6a', driver)
    for old, new in (("Tag220", f"Tag{tag}"), ("TAG220", f"TAG{tag}"), ("twin-q8-220", f"twin-q8-{tag}"),
                     ("2026111700010220", f"2026111700010{tag}")):
        text = text.replace(old, new)
    text = text.replace("# The verified pre-power logs were saved separately. These six digests\n"
                        "# pin the differing files observed and archived after BFS recovery.",
                        "# The separately verified pre-power logs remain the diagnostic evidence.\n"
                        "# Current stage digests admit restoring only from this fresh boot.")
    return text.encode("ascii")


def main():
    args = sys.argv[1:]
    tag = "222" if args and args[-1] == "--tag222" else "221"
    if tag == "222":
        args.pop()
    if not __debug__ or len(args) not in (2, 3):
        raise SystemExit("use VERIFIED_PRE_POWER_ARCHIVE NEW_LOCAL_SCRIPT [VERIFIED_POST_POWER_FILES] [--tag222]")
    post = Path(args[2]) if len(args) == 3 else None
    data = bind(Path(args[0]), post, tag)
    with Path(args[1]).open("xb") as out:
        out.write(data)
    print(sha256(data).hexdigest(), args[1])


if __name__ == "__main__":
    main()
