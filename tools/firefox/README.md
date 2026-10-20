# Firefox on Haiku/RISC-V

Experimental native Firefox 158.0 bring-up for the Pioneer distribution.
This directory contains source pins, build and packaging tools, port patches,
and native acceptance probes. The browser passed native runtime acceptance on
the Pioneer. The experimental profile has the feature limits described below.
The verified HPKG is bundled in this repository and included by default in
regular RISC-V distro images. It is installed on the native Pioneer system.

The starting point is HaikuPorts revision
`cf125b5fd31c10bfa34497f9dc8e05b4be216388`, including its native `cairo-haiku`
backend. Sources and upstream patches are downloaded rather than vendoring the
entire browser into the OS repository. See `sources.lock.json` for exact URLs
and SHA-256 digests.

## Current evidence

On October 19, 2026, the native Pioneer passed:

- Rust 1.90 standard-library bootstrap; threads, barriers, TLS, 64-bit atomics,
  file creation/write/sync/read, child processes and pipes, and TCP loopback
  with timeouts. Both ordinary and abort-profile Rust programs ran natively.
- libffi mixed integer/floating-point/pointer calls and executable closures.
- libevent descriptor callbacks, timers, and cross-thread wakeup using Haiku's
  `poll` backend.
- The bundled MessagePack copy's exact wire encoding of positive/negative
  integers and a floating-point value after fixing Haiku byte-order detection.
- NSPR architecture identification, threads, atomics, and loopback sockets.
- NSS module integrity-file generation, initialization, a SHA-256 known-answer
  test, random generation, and TLS 1.3 availability.
- Fontconfig discovery of an installed Noto font through the new FreeType build.

The aggregate native probe script exited successfully with
`FIREFOX_DEPENDENCY_PROBES_PASS`. Separately, the running native Firefox passed:

- DOM/CSS grid, JavaScript promises, canvas pixel readback, local storage,
  Web Worker messaging, and HTTP fetch, with an actual page callback.
- Synthetic WebDriver text input, a second tab, and HTTPS certificate validation
  with `acceptInsecureCerts=false`.
- Firefox's download service fetched an HTTPS page to the private profile and
  read it back successfully.
- A visible native Haiku window, inspected in a native screen capture, and a
  normal native Quit request followed by parent/child process exit.

The tested build ID is `20261019163755`; Firefox reports version 158.0,
platform `haiku`, and `moz:headless=false`. The initial black screen capture was
the idle screen blanker; ordinary pointer activity restored the visible desktop.
The final HPKG was extracted and retested through its native command link and
launcher, using packaged graphics defaults in a fresh profile. The final native
window capture was inspected and the packaged browser shut down normally.

## Host and isolated workspace

Use an ARM64 macOS host, Python 3.12+, Node.js meeting Firefox's requirement,
and a configured Haiku RISC-V build containing development/runtime packages
and the `riscv64-unknown-haiku` cross-tools.

**Both Firefox sources and object files require a case-sensitive filesystem.**
Mozilla generates `string.h` wrappers that otherwise intercept Haiku's
`String.h`. The setup and extraction scripts reject a case-insensitive work
directory. The existing `/Volumes/HaikuBuildLocal` volume is suitable.

All generated tools, source trees, libraries, and browser output belong in a
private work directory. The scripts do not replace system libraries, alter
Pioneer drivers, reboot the machine, or select a new boot image.

Example variables (use absolute paths):

```sh
export FIREFOX_WORK=/Volumes/HaikuBuildLocal/firefox-riscv64-20261019
export HAIKU_BUILD=/Volumes/HaikuBuildLocal/generated.riscv64
export HAIKU_SOURCE=/path/to/HaikuOS
export FIREFOX_PYTHON=/path/to/python3.12
export FIREFOX_NODE=/path/to/node
export RUSTUP_HOME="$FIREFOX_WORK/rustup"
export CARGO_HOME="$FIREFOX_WORK/cargo"
```

From the Haiku repository root:

```sh
"$FIREFOX_PYTHON" tools/firefox/fetch_sources.py --work "$FIREFOX_WORK"
# --offline verifies all cached inputs without downloading anything.

chmod +x "$FIREFOX_WORK/downloads/rustup-init-1.29.1"
"$FIREFOX_WORK/downloads/rustup-init-1.29.1" -y --no-modify-path \
    --profile minimal --default-toolchain 1.90.0
"$CARGO_HOME/bin/rustup" component add rust-src --toolchain 1.90.0

"$FIREFOX_PYTHON" tools/firefox/setup_cross.py \
    --build "$HAIKU_BUILD" --work "$FIREFOX_WORK" \
    --rust "$RUSTUP_HOME/toolchains/1.90.0-aarch64-apple-darwin" \
    --python "$FIREFOX_PYTHON" --node "$FIREFOX_NODE" --bootstrap-rust

"$FIREFOX_PYTHON" tools/firefox/prepare_sources.py --work "$FIREFOX_WORK" \
    firefox nspr nss libffi libevent fontconfig freetype expat pkgconf m4 autoconf

"$FIREFOX_PYTHON" tools/firefox/build_dependencies.py \
    --build "$HAIKU_BUILD" --work "$FIREFOX_WORK" --jobs 4 \
    host-tools libffi libevent nspr fonts nss

"$CARGO_HOME/bin/cargo" +1.90.0 install cbindgen --version 0.29.4 \
    --locked --root "$FIREFOX_WORK/host-tools"
```

Source preparation requires fresh component directories. It refuses to
silently re-extract or reapply patches over an existing source tree. Build
commands record their exact invocations and output under `$FIREFOX_WORK/logs`.
Cargo vendor-file checksums are recomputed after applying the Haiku patches;
the checks are retained rather than disabled.

Run the Rust bootstrap before compiling Firefox. If the generated Rust
standard library is rebuilt later, discard or rename the generated
`obj/riscv64gc-unknown-haiku` Cargo output directory before resuming Firefox;
Cargo does not reliably invalidate cached crate metadata after sysroot changes.

## Native prerequisite tests

```sh
"$FIREFOX_PYTHON" tools/firefox/build_probes.py --work "$FIREFOX_WORK" \
    --rust "$RUSTUP_HOME/toolchains/1.90.0-aarch64-apple-darwin"
```

Copy `dependency-probes.tar.gz` to a private directory on Haiku, extract it,
then run:

```sh
sh firefox-dependency-probes/run-probes.sh
```

The probe runner uses its own library directory and bounded commands. NSS
`.chk` integrity files are generated with the target-native `shlibsign`; a
cross build cannot execute that tool on macOS. No SSH credentials, machine
addresses, or private keys are included in the build tooling or archive.

## Browser compilation

```sh
. "$FIREFOX_WORK/cross-env.sh"
cd "$FIREFOX_WORK/source/firefox-158.0"
"$FIREFOX_PYTHON" mach configure
"$FIREFOX_PYTHON" mach build -j4
make -C "$FIREFOX_WORK/obj/browser/installer" stage-package
"$FIREFOX_PYTHON" "$HAIKU_SOURCE/tools/firefox/stage_browser.py" \
    --work "$FIREFOX_WORK"
```

The initial configuration is unbranded, with JIT and WebRTC disabled to reduce
the architecture bring-up surface. It uses the native Haiku toolkit, system
NSPR/NSS and libevent from the private prefix, and an explicit custom Rust target.
The Null GL provider selects software rendering without a Mesa/libGL dependency.
WebGL and GPU acceleration are unavailable, and the native feature check also
confirmed WebAssembly is unavailable without JIT. The FreeType bootstrap omits
optional compression/image/shaping dependencies;
this configuration is for bring-up, not a final distribution feature set.

Mozilla's `stage-package` prepares the browser runtime. The Python staging
helper adds private libraries, relocatable font configuration, license files,
source/build material, test tools, and checksums, then writes a test archive
without requiring GNU tar on macOS. The output explicitly records native
acceptance as pending. It refuses to overwrite an existing stage.

## Local patches

- `firefox-cross-riscv64.patch`: explicit custom Rust target selection,
  RISC-V XPCOM call stubs and platform strings, software GL provider selection,
  Skia's Haiku/POSIX platform detection, libevent include flags, Haiku pthread
  extension declarations and visibility wrappers, MessagePack's Haiku endian
  header, x86-only CPU queries, the `cc` crate's RISC-V double-float ABI,
  and macOS SDK minimum checks applied to macOS targets rather than imposing
  the browser target's SDK minimum on host tools.
- `nspr-riscv64.patch`: Haiku cross-target naming, LP64 layout, RISC-V CPU
  identification, and exclusion of x86 assembly from the RISC-V build.
- `libffi-haiku-closures.patch`: use executable mappings for Haiku closures.
  The unpatched malloc-backed trampoline failed with an execute-permission
  fault on the Pioneer; the patched native test passed.
- `native-launcher.patch`: relocatable private libraries/font configuration,
  an isolated default profile, symlink resolution, and intact forwarding of
  explicit profile paths. Applied to a copy of the pinned HaikuPorts launcher
  during package preparation.

## Distro acceptance

`browser_acceptance.py` drives an already-running private browser using
Mozilla's Marionette transport from the pinned source tree. It has no pip
dependencies and connects only to a local port, which can be forwarded over
SSH from the Haiku machine. The recorded native results above came from
running the tool and separately inspecting a native window capture.

On Haiku, use a fresh dedicated profile containing `probes/user.js`, launch
the staged browser with `--no-remote --profile /absolute/private/profile
--marionette about:blank`, and start `browser-probe-server acceptance.html`.
The server prints the native loopback URL. Forward Haiku's loopback port
28281 to a local port, then run on the build host:

```sh
"$FIREFOX_PYTHON" tools/firefox/browser_acceptance.py \
    --source "$FIREFOX_WORK/source/firefox-158.0" \
    --port 28281 --profile /absolute/private/profile \
    --url http://127.0.0.1:SERVER_PORT/ \
    --output "$FIREFOX_WORK/browser-acceptance" --quit
```

The native system clock must be correct for certificate validation. The
script requires validation to remain enabled and records the actual browser
capabilities, DOM/CSS/JavaScript/canvas/worker fixture result, synthetic input,
an HTTPS page in another tab, screenshots, and any clean shutdown request.
Verify the browser process actually exits and inspect a native window capture
separately. The output directory is preserved on failure and never overwritten.
To exercise Firefox's download service as well, launch the private test browser
with `--remote-allow-system-access` and add `--test-download` to the controller.
That test writes and removes its probe file only inside the verified test profile.

Before making this the distribution's production browser, verify a native launch,
local HTML/CSS/JavaScript rendering, HTTPS with certificate validation,
multiple tabs and child processes, input/clipboard, downloads, and clean
shutdown. Record the actual browser/package hashes and runtime limitations.
Preserve upstream license notices, source pins, and all distribution patches
with the package's corresponding source/build material.

## Experimental HPKG

After browser acceptance and visual inspection, prepare a fresh package work
directory on the build host:

```sh
"$FIREFOX_PYTHON" "$HAIKU_SOURCE/tools/firefox/prepare_package.py" \
    --work "$FIREFOX_WORK" --acceptance /path/to/acceptance.json \
    --native-capture /path/to/native-window.png
```

The helper checks the browser build ID, accepted runtime hashes, and required
test results. It adds a native launcher derived from the pinned HaikuPorts
launcher, software-rendering defaults, Deskbar/command links, licenses, and
source/build material. Its default profile is isolated under the user's
settings directory as `FirefoxExperimental`; an explicit profile takes priority.

Copy `firefox-158.0-riscv64-package-input.tar.gz` to a private Haiku directory,
extract it, and run `sh firefox-package-work/build-package.sh`. This verifies
input hashes, generates NSS integrity files natively, and creates
`firefox_experimental-158.0-1-riscv64.hpkg` using fast zlib compression. Extract
and retest that package before including it in a distro image. Package creation
does not install it globally.

The current validated artifact is in
`generated.firefox/packages/firefox_experimental-158.0-1-riscv64.hpkg`.
`verified-build.json` records its SHA-256, build identity, native results,
feature limits, and evidence paths. Generated browser sources and toolchains
stay outside Git. The complete tested HPKG is stored as three binary parts
under `packages/`, each below 50 MiB.
`python3 tools/firefox/bundled_package.py` reconstructs and verifies it without
network access or Git LFS. Rebuilt HPKGs can differ bytewise; validate and update
the parts, bundle manifest, and receipt before using a new artifact.

To include this exact package in a staged Haiku RISC-V image containing
`system/packages`, run from the repository root:

```sh
"$FIREFOX_PYTHON" tools/firefox/include_in_image.py \
    --image-root /path/to/staged/haiku-image
```

The helper verifies the recorded package hash and the copied image file,
refuses to replace a different package, and rejects the live OS package
directory. A prepared overlay is also available at
`generated.firefox/image-overlay/system/packages/`.

## Distro image and installed system

Regular RISC-V builds include this package through `tools/firefox/ImageRules`.
The Jam action assembles the repository's package parts, verifies their hashes,
and adds the HPKG to `system/packages`. Set `HAIKU_IMAGE_FIREFOX = 0 ;` in
`UserBuildConfig` to omit it, or `HAIKU_IMAGE_FIREFOX = 1 ;` to include it in a
minimum image. Use an image size of at least 768 MiB for the current font-rich
Pioneer image. The Firefox hook defaults to 768 MiB when no size is configured.
`pioneer_build.sh --image` now reads the partition and BFS geometry when
extracting the deployable payload, including images larger than 300 MiB.

The prepared image is:

```text
generated.firefox/distro-image-20261020/haiku-pioneer-firefox.image
```

It has the existing EFI partition and a 768 MiB BFS partition containing the
validated September 8 quiet-AHCI system package, fonts, and Firefox. Its system
HPKG matches the one installed on the Pioneer. `clonefs` verified filesystem
content, attributes, metadata, and indices; `checkfs` passed; Firefox was copied
back and checksum-verified. Embedded SSH identity/access settings were removed
from this redistributable output. The image has not yet been boot-tested.
Its paths, input/output hashes, and check results are in
`evidence/image-receipt.json`. Reproduce it with:

```sh
python3 tools/firefox/build_image.py \
    --bfs-shell /path/to/host/bfs_shell \
    --base-bfs /path/to/haiku-pioneer-bfs-quiet-ahci.img \
    --base-sha256 05240d2f696c8eb0809f08a7fddc051cd4aa491d7fab218a8fd71f660db0fe49 \
    --efi-image /path/to/haiku-mmc.image \
    --output /path/to/new-image-directory
```

On the native Pioneer, the package was installed system-wide using:

```sh
pkgman install -R -y /path/to/firefox_experimental-158.0-1-riscv64.hpkg
```

Launch **Applications → Firefox Experimental** from Deskbar, or run
`firefox-experimental`. The default profile is
`/boot/home/config/settings/FirefoxExperimental`. Installed-package acceptance
passed JavaScript, canvas, storage, workers, HTTP fetch, synthetic input, tabs,
certificate-validated HTTPS, and the download service. The native window was
visually checked, and the test browser quit normally. A normal launch with the
default profile loaded Example Domain and was left open for the user with
automation disabled. See `evidence/deployment.json`,
`evidence/installed-acceptance.json`, and `evidence/installed-native-window.png`.

Native physical keyboard/clipboard, download UI/file picker, media playback, printing, and
extended browsing remain separate release checks. The HTTPS sample also showed
missing Arabic glyphs with the installed fonts. Upstream Haiku startup logs
contain nonfatal argument-message and GIO telemetry warnings. Treat this as an
experimental browser package, not a completed production browser port.
