# Bundled Firefox HPKG

These three binary parts contain the complete, tested experimental Firefox
158.0 RISC-V HPKG. Each part is below 50 MiB, so ordinary Git clones contain
the package without Git LFS or a separate download. Keep all three parts.
`bundle.json` records their ordered SHA-256 digests and the complete HPKG digest.

From the repository root:

```sh
python3 tools/firefox/bundled_package.py
```

The helper verifies every part and the reconstructed package before publishing
`generated.firefox/packages/firefox_experimental-158.0-1-riscv64.hpkg`.
The HPKG contains license notices and pinned source/build material. Port source,
upstream source URLs/checksums, patches, feature limits, and reproduction steps
are in the parent directory. Replacing the browser requires a newly validated
package, new parts/manifest, and an updated `verified-build.json`.
