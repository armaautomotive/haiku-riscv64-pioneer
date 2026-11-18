#!/usr/bin/env python3
"""Bind two exact worker images and one authenticated shard; no driver or board I/O."""
import ast
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

from build_qwen38_twin_native_q8_wire import WIRE_WORKER_PINS
from generate_qwen38_token_programs import authenticated

HERE = Path(__file__).resolve().parent
FROZEN = "5a95c1c42983751153df493c2540d9b6501520d8"
RAW_ELF = "4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870"
SOURCE = "c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
WEIGHTS = ("a6a28c1abe3d8475b90c2fb6c79b796a19e3f5ecce0f96066cdd41a59f0aa005",
           "a3fa3db0700afe50fc55e60b0b525a84a96343a14a2a11fca8713bf34197a68c")
SECTIONS = (("globalworker", 0x20000), ("precise", 0x30000), ("dense", 0x40000),
            ("ffn", 0x60000), ("service", 0x68000), ("fetch", 0x70000),
            ("affine", 0x70B00), ("text", 0x72000), ("gdn", 0xB8000), ("boot", 0x6000))


def raw_pins():
    source = subprocess.check_output(["git", "-C", HERE.parents[1], "show",
                                     FROZEN + ":tools/pioneer/build_qwen38_vector_q4_metadata_worker.py"])
    value, = [ast.literal_eval(n.value) for n in ast.parse(source).body if isinstance(n, ast.Assign)
              and any(isinstance(t, ast.Name) and t.id == "PINS" for t in n.targets)]
    assert ("program.elf", 144592, RAW_ELF) in value
    return value


def sections(elf):
    assert elf[:6] == bytes((127, 69, 76, 70, 1, 1)) and struct.unpack_from("<H", elf, 18)[0] == 243
    offset, = struct.unpack_from("<I", elf, 32)
    size, total, names = struct.unpack_from("<HHH", elf, 46)
    assert size == 40 and offset + total * size <= len(elf) and names < total
    table = [struct.unpack_from("<10I", elf, offset + i * size) for i in range(total)]
    pool = table[names]
    strings = elf[pool[4]:pool[4]+pool[5]]
    result = {}
    for name, kind, flags, address, start, count, *_ in table:
        end = strings.find(bytes(1), name)
        assert end >= name
        name = strings[name:end].decode("ascii")
        if kind == 1 and flags & 2:
            assert start + count <= len(elf) and name.startswith(".") and name[1:] not in result
            result[name[1:]] = (address, elf[start:start+count])
    return result


def emit_words(name, contents):
    assert contents and not len(contents) % 4
    words = struct.unpack("<" + "I" * (len(contents) // 4), contents)
    rows = [",".join(f"0x{w:08x}U" for w in words[i:i+8]) for i in range(0, len(words), 8)]
    return f"static const uint32_t {name}[]={{\n" + ",\n".join(rows) + "\n};\n"


def main():
    if len(sys.argv) != 5 or not __debug__:
        raise SystemExit("use ABSOLUTE_PINNED_RAW_WORKER ABSOLUTE_PINNED_Q8_WIRE_WORKER ABSOLUTE_GGUF NEW_OUTPUT")
    raw, native, model, out = map(Path, sys.argv[1:])
    assert all(p.is_absolute() for p in (raw, native, model, out))
    assert not any(k.startswith(("PIONEER_", "TT_PIONEER_")) for k in os.environ)
    records = zip((raw, native), (raw_pins(), WIRE_WORKER_PINS))
    arms = [authenticated(path, pins) for path, pins in records]
    for arm in arms:
        linked = sections(arm["program.elf"])
        assert set(linked) == {name for name, _ in SECTIONS}
        for name, address in SECTIONS:
            assert linked[name] == (address, arm[name + ".bin"]), name
        for pc in (0x20060, 0x2009c):
            assert arm["globalworker.bin"][pc-0x20000:pc-0x20000+4] == b"\x6f\x00\x00\x00"
    out.mkdir(mode=0o700)
    unit = out / "export-fixture"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2", "-g", "-Wall", "-Wextra", "-Werror",
                    "-Wno-deprecated-declarations", "-fsanitize=address,undefined",
                    HERE / "pioneer_q38_twin_q8_fixture_export.cpp", HERE / "PioneerModelWeightCache.cpp",
                    "-o", unit], cwd=out, check=True)
    fixture = out / "pair-pages.bin"
    subprocess.run([unit, model, fixture], cwd=out, check=True, env={**os.environ, "UBSAN_OPTIONS": "halt_on_error=1"})
    data = fixture.read_bytes()
    assert len(data) == 2 * 68 * 4096
    slices = [data[i*68*4096:(i+1)*68*4096] for i in range(2)]
    assert tuple(sha256(part).hexdigest() for part in slices) == WEIGHTS
    text = ('/* SPDX-License-Identifier: MIT */\n'
            '// Private generated diagnostic payload. Original model fixture, not an executable driver.\n'
            '#if !defined(_KERNEL_MODE) || !defined(PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE) \\\n'
            ' || PIONEER_Q38_TWIN_Q8_NATIVE_APERTURE != 1 || defined(PIONEER_Q38_WORK_INFERENCE) \\\n'
            ' || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE)\n#error Exact private Q8 diagnostic payload role required\n#endif\n'
            '#include "PioneerQ38TwinQ8L1Lease.h"\nnamespace PioneerQ38TwinQ8Payload {\n'
            'using Arm=PioneerQ38TwinQ8ProjectionSequence::Arm;\n')
    for side, arm in enumerate(arms):
        for index, (name, _) in enumerate(SECTIONS):
            text += emit_words(f"Section{side}_{index}", arm[name + ".bin"])
        text += emit_words(f"Weights{side}", slices[side])
    text += ('struct ROM {\n static constexpr unsigned WaitPC=0x2004c, StopPC=0x20060, FaultPC=0x2009c;\n'
             ' bool Section(Arm arm,unsigned index,const uint32_t*& data,unsigned& words)const{\n'
             '  if(arm!=Arm::Raw && arm!=Arm::Native)return false;\n  switch(index){\n')
    for i in range(10):
        text += (f"  case {i}: if(arm==Arm::Raw){{data=Section0_{i};words=sizeof(Section0_{i})/4;}}"
                 f"else{{data=Section1_{i};words=sizeof(Section1_{i})/4;}}return true;\n")
    text += ('  default:return false;\n  }\n }\n'
             ' const uint32_t* WeightWords(Arm arm)const{\n'
             '  return arm==Arm::Raw?Weights0:arm==Arm::Native?Weights1:nullptr;\n }\n};\n'
             'static_assert(sizeof(Weights0)==4096*PioneerQ38TwinQ8ProjectionSequence::WeightPages\n'
             ' && sizeof(Weights1)==sizeof(Weights0),"complete canonical pages");\n}\n')
    payload = out / "PioneerQ38TwinQ8Payload.h"
    payload.write_text(text)
    manifest = {
        "source_sha256": SOURCE, "weight_sha256": WEIGHTS, "page_count": 68,
        "payload_sha256": sha256(payload.read_bytes()).hexdigest(),
        "elf_sha256": [sha256(arm["program.elf"]).hexdigest() for arm in arms],
        "section_sha256": [{s: sha256(arm[s + ".bin"]).hexdigest() for s, _ in SECTIONS} for arm in arms],
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(manifest["payload_sha256"], payload)
    print("PASS two complete linked worker images; fixed 68-page arms; no driver or hardware")


if __name__ == "__main__":
    main()
