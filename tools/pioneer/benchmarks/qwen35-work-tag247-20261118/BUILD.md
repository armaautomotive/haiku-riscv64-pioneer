# Qwen3.5-4B Work tag247: accept the Work fault park

Tag `2026111800010047` corrects the peer fault park used by rebootless Work
recovery. The shared persistent peer loop is at `0x7209c`, but the Work peer
stores its faulting call site before publishing Q8PF. That adds a store and a
fence, moving this image's terminal self-loop to `0x720a4`. Tag246 only
recognized the generic address, so it rejected the otherwise explicit,
parked Q8PF peer as proof reason17. Tag247 admits the Work-specific instruction
range while waiting and requires two PC reads at the exact `0x720a4` self-loop
before resetting the fleet.

## Offline build

Built with the local Haiku RISC-V64 toolchain:

```sh
python3 -B tools/pioneer/build_qwen35_work.py \
  /Volumes/HaikuBuildLocal/generated.riscv64 /private/tmp/pioneer-qwen35-work-offline-247b
```

The Work owner regression compiled with `-Wall -Wextra -Werror` and passed,
including exact fault-handler/park PC checks and the existing peer ledger,
marker, return-PC and panel-count receipt checks. The built peer disassembly
confirms its self-branch is at `0x720a4`. `git diff --check`, Python AST
parsing, and build-script shell syntax checks passed.

| Artifact | SHA-256 |
| --- | --- |
| Candidate driver55 | `6a556dfd0ca15bbfa5864171d320d2ae106183e3d3ac9d9317c37f82c6d206ef` |
| Reload guard | `d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229` |
| Work client | `8b51d7cd0810c55fca082aa970621a74da72319b2ceba12c8a2dae45cd4a6acb` |
| ROOT image boot | `32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca` |
| ROOT image text | `7cb5bbc499639721a678f49225cc3347ed4e0c09f020fd84f26e95c289a33fd3` |
| Peer image boot | `e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b` |
| Peer image text | `feef2e513367a83f93c11142037c59e2a6af5aed1d7921d9cf031daa4fe63047` |

## Hardware result

Stable041 was restored idle at generation2 before the run. Tag247 authenticated
in `100.777613` seconds, tokenized the prompt in `14.373964` seconds, and
uploaded/loaded the model in `225.068757` seconds. ROOT faulted during prefill
with zero committed tokens. The first safe-idle proof returned reason17,
detail `0x4503851`; the STOP reproof returned reason17, detail `0x1503851`.
Both details identify Q8PF, now from workers4 and1, so the `0x720a4` park
recognition alone did not clear the status proof. The wrapper could not unload
the candidate; it remains quarantined at generation3 `cold1 active_owner1`.
No output or decode rate was produced. Raw logs and state are preserved in
`evidence/`.

Tag248 adds two explicit, sealed fault-loop branches so the speculative PC
read may report either safe word, and returns field-specific proof codes for
the peer status ledger, marker, return PC, receipt, and extra words. The
27B model and stable041 backup remain preserved.
