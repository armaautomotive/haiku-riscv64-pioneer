#!/usr/bin/env python3
"""Actual whole native context/ABI instantiation, refusing object ONLY.

Uses an authenticated prior SDK compiler trace and recompiles the actual
old65 control first. Never links, installs, opens a device or grants Work.
"""
import hashlib
import os
import pathlib
import re
import shlex
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith('PIONEER_') and v != '0' for k, v in os.environ.items()):
    raise SystemExit(2)
qualified, output = map(pathlib.Path, sys.argv[1:])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
old52 = qualified/'old52/tenstorrent_pci_probe-driver55-gdn-rendezvous-debug-tag52'
assert hashlib.sha256(old52.read_bytes()).hexdigest() == 'cdacaa3a971823cbc5bbbebde8bd2c3771d48842df814729b5fe67c4dc290759'
traces = list(qualified.rglob('old-tag38-build.trace'));assert len(traces) == 1
here = pathlib.Path(__file__).resolve().parent
project = here.parents[2]
commands = [shlex.split(line[2:]) for line in traces[0].read_text().splitlines() if line.startswith('+ ')]
compiles = [a for a in commands if '-c' in a and a[a.index('-c')+1].endswith('/src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp')
            and '-DTT_PIONEER_DRIVER_RELOAD' in a]
assert len(compiles) == 1
original = compiles[0]
assert original[0] == '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert not any(a.startswith(('-fplugin','-wrapper','@','-Wl,')) or a == '-shared' for a in original)
original_project = str(pathlib.Path(original[original.index('-c')+1]).parents[5])
assert original_project.endswith('/HaikuOS')
args = [a.replace(original_project,str(project)) for a in original
        if not a.startswith(('-DPIONEER_Q38_FFN_HOST_PROFILE','-DPIONEER_Q38_FFN_SEALED_PROOF'))]
tags = [i for i,a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')];assert len(tags) == 1
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
output.mkdir(mode=0o700)
old = output/'unchanged-native65.o'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010065ULL'
args[args.index('-c')+1] = str(here/'pioneer_q38_next_ffn_residual_native_full_compile.cpp')
args[args.index('-o')+1] = str(old)
subprocess.run(args,check=True,env=clean)
assert hashlib.sha256(old.read_bytes()).hexdigest() == '61faa093c5f247f8f120a6712a24c7e5ef6989c6dc8aeb7466241dca6d52a821','old65 MUST remain byte-exact'
obj = output/'resident-output-context-REFUSING-COMPILE-ONLY.o'
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x2026102500010067ULL'
args[args.index('-c')+1] = str(here/'pioneer_q38_resident_output_native_context_full_compile.cpp')
args[args.index('-o')+1] = str(obj)
subprocess.run(args,check=True,env=clean)


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split('\t');assert len(parts) == 3
        match = re.fullmatch(r'(.*?):\d+:\d+:(.*)',parts[0]);assert match
        key = str(pathlib.Path(match[1]).resolve())+':'+match[2]
        assert parts[2] == 'static'
        result[key] = max(result.get(key,0),int(parts[1]))
    return result


current, previous = frames(obj.with_suffix('.su')),frames(old.with_suffix('.su'));assert current
for name,size in current.items():
    assert size <= 2048 or name in previous and size <= previous[name],(name,size,previous.get(name))
sizes = [size for name,size in current.items() if 'ResidentOutput' in name or 'pioneer_output_context_' in name]
assert sizes and max(sizes) <= 2048
for method in ('Prepare','Run','Close'):
    concrete = [size for name,size in current.items()
                if 'PioneerQ38ResidentOutputOwner::'+method+'(' in name and '[with IO = PioneerSessionIO]' in name]
    assert concrete and max(concrete) <= 2048,(method,'actual IO controller instantiation must not disappear')
toolchain = pathlib.Path(args[0]).parent
symbols = subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-nm'),'-S','--defined-only',str(obj)],text=True)
defined = {}
for line in symbols.splitlines():
    row = line.split()
    if len(row) == 4 and re.fullmatch('[0-9a-f]+',row[0]) and re.fullmatch('[0-9a-f]+',row[1]):
        defined[row[3]] = (int(row[0],16),int(row[1],16),row[2])
normal = ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')
assert not set(normal) & set(defined),'NO conventional driver ABI definitions permitted'
assert all('pioneer_q38_output_context_only_'+name in defined for name in normal)
table = subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-objdump'),'-t',str(obj)],text=True)
sections = {row[-1]:row[-3] for row in map(str.split,table.splitlines()) if len(row)>=6 and row[-1] in defined}
# COMDAT sections have independent address-zero namespaces. Never combine
# their opcodes with .text merely because their numeric addresses overlap.
assembly = subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-objdump'),'-d','-j','.text',str(obj)],text=True)
instructions = {}
for line in assembly.splitlines():
    match = re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+',line)
    if match:
        instructions[int(match[1],16)] = match[2]
relocation_table = subprocess.check_output([str(toolchain/'riscv64-unknown-haiku-objdump'),'-r','-j','.text',str(obj)],text=True)
relocations = {int(match[1],16) for line in relocation_table.splitlines()
               for match in [re.match(r'^([0-9a-f]+)\s+R_',line)] if match}


def code(name):
    start,size,kind = defined[name];assert kind in ('T','t') and size
    assert sections.get(name) == '.text',(name,'entry/control must belong to the audited section')
    assert not any(start <= at < start+size for at in relocations),(name,'entry/control cannot contain a linker rewrite')
    rows = [(address,word) for address,word in sorted(instructions.items()) if start <= address < start+size]
    cursor = start
    for address,word in rows:
        assert address == cursor and len(word) in (4,8),(name,'missing/overlapping instruction bytes')
        cursor += len(word)//2
    assert cursor == start+size,(name,'EVERY declared function byte must be audited')
    return [word for _,word in rows]


refusal = code('pioneer_output_context_expected_refusal')
null = code('pioneer_output_context_expected_null')
inert = code('pioneer_output_context_expected_inert')
assert refusal and null and inert
for name in ('init_hardware','init_driver'):
    assert code('pioneer_q38_output_context_only_'+name) == refusal,(name,'entry must be the constant B_NOT_ALLOWED return')
for name in ('publish_devices','find_device'):
    assert code('pioneer_q38_output_context_only_'+name) == null,(name,'entry must return only NULL')
assert code('pioneer_q38_output_context_only_uninit_driver') == inert,'uninit must be inert'
for name in ('bind','initialize','prefix','execute','end','destroy_clean'):
    assert 'pioneer_output_context_'+name in defined
print('PASS ACTUAL whole PioneerSessionIO/PioneerStoreContext/KernelExport ABI/native factories+all controller templates/clean destructor/retention+END compile;maxNewFrame='+str(max(sizes)))
print('PASS NO conventional driver ABI exports;ALL5 renamed entries machine-code-identical to inert constant-return controls;NO PCI/module/guard/sem/card IO from entries')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+'  '+str(obj))
print('Old65 BYTE-IDENTICAL61faa093;REFUSING COMPILE OBJECT ONLY;NO link/install/public compute/Work/CPU-reference/device/speed qualification')
