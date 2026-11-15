#!/usr/bin/env python3
"""Generate immutable routed-token root/worker image DATA from pinned offline builds."""
import hashlib
import pathlib
import struct
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: build_qwen38_routed_token_programs.py ROOT_IMAGE_DIR WORKER_IMAGE_DIR OUTPUT_HEADER")
root, worker, output = map(pathlib.Path, sys.argv[1:])
assert root.is_absolute() and worker.is_absolute() and output.is_absolute()
assert not output.exists() and not output.is_symlink()

root_sections = (
    ("RootBoot", root / "boot.bin", 4, "32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca"),
    ("RootText", root / "text.bin", 62404, "28ce6c204ec35cdbeb9700230a6a268d2081d434615efc37024dfb5b2d2b97b0"),
    ("RootConv", root / "conv.bin", 7500, "705770a711c5c738c8e377c68ecb26c27095550bc9801d3089cf1e758a2940c0"),
)
worker_sections = (
    ("PeerBoot", worker / "boot.bin", 4, "a12af9d63593a1f882750a147dd7a511ac476c5c7a55526bd4d51c685173991c"),
    ("PeerMixed", worker / "mixed.bin", 4400, "96b7cdfa3785710704fcc6aca909722786e9e96dedb9a16d6d6ee7c22cba6451"),
    ("PeerDense", worker / "dense.bin", 10332, "cef405cc7405eea6001e47e9a6c9db73ca790bcac985f09d2bb964de8c68045f"),
    ("PeerFfn", worker / "ffn.bin", 16456, "f6ef5d614091b5ed12365e1945daa2dad04d37c432ac9dd5f859f857bba83e52"),
    ("PeerService", worker / "service.bin", 9888, "b9fb256a75546dfe9be4a7805ef170913b2b1eecab1fbf1afb3b6d7c098c9262"),
    ("PeerAffine", worker / "affine.bin", 5360, "47c52fee6ccfb0a8847fe17a9d43d4016e7cfc6c68cc1427673f6869cd287b78"),
    ("PeerText", worker / "text.bin", 5984, "3a8ffe4f7e885f992f210bd1cc0887a1309159972409403b3082535b1916dab1"),
    ("PeerGdn", worker / "gdn.bin", 40256, "d97b6ceb60d469002c5216c15ae57eb10b34f027de965342c9e545357ca51c6d"),
)


def load(section):
    name, path, size, digest = section
    data = path.read_bytes()
    assert len(data) == size and not (size & 3), (name, len(data), size)
    assert hashlib.sha256(data).hexdigest() == digest, name
    return name, struct.unpack("<" + "I" * (size // 4), data)


arrays = [load(section) for section in (*root_sections, *worker_sections)]
with output.open("x", encoding="utf-8") as stream:
    stream.write("/* SPDX-License-Identifier: MIT */\n")
    stream.write("/* Generated from pinned offline QMW1/RGB1 candidate images. DATA only. */\n")
    stream.write("#ifndef PIONEER_Q38_ROUTED_TOKEN_PROGRAMS_H\n#define PIONEER_Q38_ROUTED_TOKEN_PROGRAMS_H\n")
    stream.write("#include <stdint.h>\n#include <stddef.h>\n#include <limits.h>\n#include \"PioneerQ38TokenFootprint.h\"\n")
    stream.write("namespace PioneerQ38TokenPrograms {\n")
    stream.write("enum {Family=0x31575251,Version=1,CancelBoundary=1,Workers=120};\n")
    for name, words in arrays:
        stream.write("static const uint32_t " + name + "[]={\n")
        for at in range(0, len(words), 8):
            stream.write(" " + ",".join("0x%08xU" % value for value in words[at:at + 8]) + ",\n")
        stream.write("};\nstatic_assert(sizeof(" + name + ")==" + str(len(words) * 4) + ",\"pinned routed image bytes\");\n")
    stream.write("struct Span {uint32_t address,words;};\n")
    stream.write("static inline unsigned CodeSpans(unsigned w){return w>=Workers?0:w?8:3;}\n")
    stream.write("static inline Span CodeSpan(unsigned w,unsigned s){if(w>=Workers||s>=CodeSpans(w))return {UINT32_MAX,0};")
    stream.write("if(!w){switch(s){case 0:return {0x6000,sizeof(RootBoot)/4};case 1:return {0x40000,sizeof(RootText)/4};default:return {0x64000,sizeof(RootConv)/4};}}")
    stream.write("switch(s){case 0:return {0x6000,sizeof(PeerBoot)/4};case 1:return {0x20000,sizeof(PeerMixed)/4};")
    stream.write("case 2:return {0x40000,sizeof(PeerDense)/4};case 3:return {0x60000,sizeof(PeerFfn)/4};")
    stream.write("case 4:return {0x68000,sizeof(PeerService)/4};case 5:return {0x70b00,sizeof(PeerAffine)/4};")
    stream.write("case 6:return {0x72000,sizeof(PeerText)/4};default:return {0xb8000,sizeof(PeerGdn)/4};}}\n")
    stream.write("static inline unsigned CodeWords(unsigned w){unsigned n=0;for(unsigned i=0;i<CodeSpans(w);i++)n+=CodeSpan(w,i).words;return n;}\n")
    stream.write("static inline uint32_t CodeAddress(unsigned w,unsigned n){if(w>=Workers)return UINT32_MAX;for(unsigned i=0;i<CodeSpans(w);i++){Span s=CodeSpan(w,i);if(n<s.words)return s.address+n*4;n-=s.words;}return UINT32_MAX;}\n")
    stream.write("static inline bool ProgramWord(unsigned w,uint32_t a,uint32_t& out){if(w>=Workers||(a&3)||((uintptr_t)&out&3))return false;const uintptr_t p=(uintptr_t)&out;")
    for name, _ in arrays:
        stream.write("if(p>=(uintptr_t)" + name + "&&p-(uintptr_t)" + name + "<sizeof(" + name + "))return false;")
    stream.write("for(unsigned i=0;i<CodeSpans(w);i++){Span s=CodeSpan(w,i);if(a<s.address||a-s.address>=s.words*4)continue;const uint32_t* d=0;")
    stream.write("if(!w){switch(i){case 0:d=RootBoot;break;case 1:d=RootText;break;default:d=RootConv;break;}}else switch(i){")
    for index, name in enumerate(name for name, _ in arrays[3:]):
        stream.write("case " + str(index) + ":d=" + name + ";break;")
    stream.write("}if(!d)return false;out=d[(a-s.address)/4];return true;}return false;}\n}")
    stream.write("\n#endif\n")
print("PASS generated pinned routed token images: root 3 spans/" + str(sum(len(words) for _, words in arrays[:3]))
    + " words, peers 8 spans/" + str(sum(len(words) for _, words in arrays[3:]))
    + " words; no native selection or execution")
