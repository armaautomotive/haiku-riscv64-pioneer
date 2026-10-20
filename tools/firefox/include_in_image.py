#!/usr/bin/env python3
"""Copy the verified Firefox HPKG into an explicitly supplied distro image root."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

from bundled_package import assemble


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def main():
    here = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image-root", type=Path, required=True,
                        help="Staged Haiku RISC-V image root containing system/packages")
    parser.add_argument("--manifest", type=Path, default=here / "verified-build.json")
    parser.add_argument("--package", type=Path,
                        help="Defaults to the verified artifact under generated.firefox/packages")
    args = parser.parse_args()
    image = args.image_root.resolve()
    if image in (Path("/"), Path("/boot")):
        parser.error("Use a staged image root; this helper does not install into a live OS")
    destination = image / "system/packages"
    if destination.resolve() == Path("/boot/system/packages").resolve():
        parser.error("The destination resolves to the live OS package directory")
    if not destination.is_dir():
        parser.error(f"Missing image package directory: {destination}")
    manifest = json.loads(args.manifest.read_text())
    if manifest.get("tests", {}).get("extracted_package") != "pass":
        parser.error("The package must pass extracted-package acceptance first")
    package = manifest["package"]
    filename = package["filename"]
    if (Path(filename).name != filename or not filename.endswith("-riscv64.hpkg")
            or package.get("architecture") != "riscv64"):
        parser.error("Expected a verified RISC-V HPKG filename")
    source = args.package or assemble()
    if digest(source) != package["sha256"]:
        parser.error(f"Package checksum does not match the verified build: {source}")
    target = destination / filename
    if target.exists():
        if digest(target) == package["sha256"]:
            print(f"Verified Firefox package already present: {target}")
            return
        parser.error(f"Refusing to overwrite a different package: {target}")
    created = False
    try:
        with source.open("rb") as package_input, target.open("xb") as package_output:
            created = True
            shutil.copyfileobj(package_input, package_output, length=1024 * 1024)
        if digest(target) != package["sha256"]:
            raise RuntimeError("Image package readback checksum failed")
    except FileExistsError:
        raise
    except Exception:
        if created:
            target.unlink(missing_ok=True)
        raise
    print(f"Verified experimental Firefox package included in image: {target}")


if __name__ == "__main__":
    main()
