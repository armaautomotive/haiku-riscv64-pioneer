#!/usr/bin/env python3
"""Set up an isolated, experimental Firefox/Haiku RISC-V cross build.

Does not install a browser, change a distro image, or access a remote machine.
The optional Rust bootstrap writes only inside the supplied work directory.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def write_executable(path, command):
    path.write_text("#!/bin/sh\nexec " + shlex.join(command) + ' "$@"\n')
    path.chmod(0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True,
                        help="Configured Haiku RISC-V build directory")
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--rust", type=Path, required=True,
                        help="Isolated Rust 1.90 toolchain directory inside --work")
    parser.add_argument("--python", type=Path, required=True,
                        help="Python 3.10 or newer")
    parser.add_argument("--node", type=Path, required=True)
    parser.add_argument("--bootstrap-rust", action="store_true",
                        help="Build std and the native Rust probe; needs rust-src and crates.io")
    args = parser.parse_args()
    build, work, rust = (p.resolve() for p in (args.build, args.work, args.rust))
    sdk = build / "objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop"
    runtime = build / "objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku.hpkg/contents/lib"
    cross = build / "cross-tools-riscv64/bin/riscv64-unknown-haiku"
    for path in (sdk / "lib/start_dyn.o", runtime / "libroot.so",
                 Path(str(cross) + "-gcc"), Path(str(cross) + "-g++"),
                 rust / "bin/rustc", rust / "bin/cargo", args.python, args.node):
        if not path.is_file():
            parser.error(f"Missing prerequisite: {path}")
    if work not in rust.parents:
        parser.error("--rust must be an isolated toolchain inside --work")
    with tempfile.TemporaryDirectory(prefix=".firefox-case-check-", dir=work) as temporary:
        directory = Path(temporary)
        (directory / "case-probe").touch()
        if (directory / "CASE-PROBE").exists():
            parser.error("Firefox's source and object directories require a case-sensitive filesystem")
    version = subprocess.check_output([rust / "bin/rustc", "--version"], text=True)
    if not version.startswith("rustc 1.90.0 "):
        parser.error("This experimental target was validated with Rust 1.90.0 only")
    host = subprocess.check_output([rust / "bin/rustc", "-vV"], text=True)
    host = next(line.split(": ", 1)[1] for line in host.splitlines() if line.startswith("host: "))
    if host != "aarch64-apple-darwin":
        parser.error("This cross-build setup currently supports an ARM64 macOS host")
    gcc_libraries = sorted((build / "build_packages").glob("gcc_syslibs-*/lib"))
    if len(gcc_libraries) != 1:
        parser.error("Expected exactly one gcc_syslibs runtime in the configured build")
    for directory in ("wrappers", "logs", "state", "prefix", "host-tools/bin"):
        (work / directory).mkdir(parents=True, exist_ok=True)

    # Keep the SDK after Mozilla's -I system wrappers. Those wrappers restore
    # public symbol visibility around target API declarations.
    includes = ["-isystem", str(sdk / "headers"), "-isystem", str(sdk / "headers/bsd"),
                "-idirafter", str(sdk / "headers/posix")]
    for path in [sdk / "headers/os", *sorted((sdk / "headers/os").glob("*")),
                 sdk / "headers/os/add-ons/graphics"]:
        if path.is_dir():
            includes += ["-isystem", str(path)]
    link = ["-B" + str(sdk / "lib") + "/", "-L" + str(sdk / "lib"),
            "-L" + str(gcc_libraries[0]), "-L" + str(runtime)]
    for name, compiler in (("cc", "gcc"), ("cxx", "g++")):
        write_executable(work / "wrappers" / name,
                         [str(cross) + "-" + compiler, *includes, *link])
    lld = work / "wrappers/ld64.lld"
    if lld.is_symlink():
        lld.unlink()
    elif lld.exists():
        parser.error(f"Refusing to replace a non-symlink: {lld}")
    lld.symlink_to(rust / f"lib/rustlib/{host}/bin/rust-lld")

    target = {
        "arch": "riscv64", "cpu": "generic-rv64", "os": "haiku",
        "llvm-target": "riscv64-unknown-haiku", "llvm-abiname": "lp64d",
        "data-layout": "e-m:e-p:64:64-i64:64-i128:128-n32:64-S128",
        "target-family": ["unix"], "target-pointer-width": "64",
        "features": "+m,+a,+f,+d,+c,+zicsr,+zifencei",
        "code-model": "medium", "abi-return-struct-as-int": True,
        "max-atomic-width": 64, "dynamic-linking": True,
        "position-independent-executables": False, "plt-by-default": False,
        "crt-objects-fallback": "false", "relro-level": "full",
        "linker-flavor": "gnu-cc", "linker": str(cross) + "-gcc",
        "pre-link-args": {"gnu-cc": link},
        "metadata": {"description": "Experimental Haiku RISC-V Firefox bootstrap",
                     "host_tools": False, "std": False, "tier": 3},
    }
    (work / "riscv64gc-unknown-haiku.json").write_text(json.dumps(target, indent=2) + "\n")
    env_values = {
        "RUSTC": str(rust / "bin/rustc"), "CARGO": str(rust / "bin/cargo"),
        "RUST_TARGET_PATH": str(work), "CC": str(work / "wrappers/cc"),
        "CXX": str(work / "wrappers/cxx"), "HOST_CC": "/usr/bin/clang",
        "HOST_CXX": "/usr/bin/clang++", "NODEJS": str(args.node.resolve()),
        "PKG_CONFIG_LIBDIR": str(work / "prefix/lib/pkgconfig"),
        "MOZBUILD_STATE_PATH": str(work / "state"),
        "MACH_BUILD_PYTHON_NATIVE_PACKAGE_SOURCE": "none",
    }
    gcc_version = subprocess.check_output([str(cross) + "-g++", "-dumpfullversion"], text=True).strip()
    cpp_headers = cross.parent.parent / "riscv64-unknown-haiku/include/c++" / gcc_version
    bindgen_flags = ["-nostdinc++", *includes, "-isystem", str(cpp_headers),
                     "-isystem", str(cpp_headers / "riscv64-unknown-haiku"),
                     "-isystem", str(cpp_headers / "backward")]
    env_values["BINDGEN_CFLAGS"] = shlex.join(bindgen_flags)
    path_prefix = [work / "wrappers", work / "host-tools/bin", cross.parent]
    export_lines = [f"export {key}={shlex.quote(value)}" for key, value in env_values.items()]
    export_lines += ["export PATH=" + shlex.quote(":".join(map(str, path_prefix))) + ':"$PATH"']
    options = ["--target=riscv64-unknown-haiku", "--enable-default-toolkit=cairo-haiku",
               "--with-gl-provider=Null",
               "--enable-audio-backends=haiku", "--with-rust-target=riscv64gc-unknown-haiku",
               "--with-system-nspr", "--with-system-nss", "--disable-tests",
               "--with-system-libevent",
               "--disable-updater", "--disable-crashreporter", "--disable-webrtc",
               "--disable-jit", "--disable-debug", "--disable-debug-symbols",
               "--disable-lto", "--without-wasm-sandboxed-libraries"]
    mozconfig = work / "mozconfig.riscv64"
    mozconfig.write_text("# Experimental unbranded bring-up, not a distro release configuration.\n"
                        + "mk_add_options MOZ_OBJDIR=" + shlex.quote(str(work / "obj")) + "\n"
                        + "\n".join("ac_add_options " + option for option in options) + "\n"
                        + "\n".join(export_lines) + "\n")
    env_values["MOZCONFIG"] = str(mozconfig)
    (work / "cross-env.sh").write_text("\n".join(export_lines)
        + "\nexport MOZCONFIG=" + shlex.quote(str(mozconfig)) + "\n")

    if args.bootstrap_rust:
        rust_source = rust / "lib/rustlib/src/rust/library/Cargo.lock"
        if not rust_source.is_file():
            parser.error("Install the matching rust-src component first")
        environment = dict(os.environ, **env_values)
        environment.update(RUSTC_BOOTSTRAP="1", CARGO_TARGET_DIR=str(work / "rust-bootstrap"))
        probe = Path(__file__).resolve().parent / "probes/rust-smoke/Cargo.toml"
        subprocess.run([rust / "bin/cargo", "-Z", "build-std=std,panic_abort", "build",
                        "--manifest-path", probe, "--target", "riscv64gc-unknown-haiku"],
                       env=environment, check=True)
        # Only this private target directory is replaced; the host toolchain stays usable.
        destination = rust / "lib/rustlib/riscv64gc-unknown-haiku/lib"
        destination.mkdir(parents=True, exist_ok=True)
        for library in destination.glob("*.rlib"):
            library.unlink()
        for library in (work / "rust-bootstrap/riscv64gc-unknown-haiku/debug/deps").glob("*.rlib"):
            if not library.name.startswith("libhaiku_firefox_rust_smoke-"):
                shutil.copy2(library, destination / library.name)
        # build-std follows the probe's default unwind profile even for the
        # panic_abort crate. Firefox also links abort-profile Rust libraries;
        # this runtime must itself be compiled with -Cpanic=abort, while std
        # and core remain the ordinary unwind-compatible libraries.
        for library in destination.glob("libpanic_abort-*.rlib"):
            library.unlink()
        subprocess.run([rust / "bin/rustc", "--crate-name", "panic_abort", "--edition=2024",
                        "--crate-type", "rlib", "--target", "riscv64gc-unknown-haiku",
                        "-C", "panic=abort", "-C", "opt-level=2",
                        "-C", "metadata=haiku_firefox_panic_abort",
                        rust / "lib/rustlib/src/rust/library/panic_abort/src/lib.rs",
                        "-o", destination / "libpanic_abort-haiku_firefox.rlib"],
                       env=environment, check=True)
    print(f"Cross environment: {work / 'cross-env.sh'}")
    print(f"Firefox mozconfig: {mozconfig}")
    print("No browser has been built or added to the distro by this setup step.")


if __name__ == "__main__":
    main()
