#!/usr/bin/env python3
"""Emit immutable full-token+QGW1 DATA from fixed artifacts. NEVER install/run."""
import ast
import hashlib
import os
import pathlib
import stat
import sys

def require(condition, message):
    if not condition:
        raise ValueError(message)

def pins(path, key):
    tree = ast.parse(path.read_text())
    found = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == key for t in n.targets)]
    require(len(found) == 1 and found[0], f'fixed {key} pin set required')
    return found[0]

def authenticated(directory, records):
    result = {}
    for name, size, digest in records:
        with (directory/name).open('rb') as source:
            before = os.fstat(source.fileno())
            require(stat.S_ISREG(before.st_mode) and before.st_size == size, f'artifact {name}: type/length')
            data = source.read(size+1)
            after = os.fstat(source.fileno())
            identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
            require(identity(before) == identity(after) and len(data) == size, f'artifact {name}: changed source')
        require(hashlib.sha256(data).hexdigest() == digest, f'artifact {name}: fixed SHA256')
        result[name] = data  # emit THIS same immutable authenticated snapshot;never reopen
    return result

def main():
    require(len(sys.argv) == 5 and sys.argv[4] in ('--cancel-boundary', '--no-cancel'), 'explicit exact FP32 token variant required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role overrides refused')
    root, worker, output = map(pathlib.Path, sys.argv[1:4])
    require(all(p.is_absolute() for p in (root, worker, output)) and not output.exists() and not output.is_symlink(), 'new absolute output required')
    here = pathlib.Path(__file__).resolve().parent
    cancel = sys.argv[4] == '--cancel-boundary'
    r = authenticated(root, pins(here/'build_qwen38_token_root.py', 'PINS_CANCEL_F32' if cancel else 'PINS_F32'))
    w = authenticated(worker, pins(here/'build_qwen38_global_worker_image.py', 'PINS'))
    units = [(r, 'boot', 'RootBoot', 0x6000), (r, 'text', 'RootText', 0x40000),
             (r, 'terminal', 'RootTerminal', 0x60000), (r, 'conv', 'RootAux', 0x64000)]
    units += [(w, name, label, address) for name, label, address in (
        ('boot', 'PeerBoot', 0x6000), ('globalworker', 'PeerFront', 0x20000), ('dense', 'PeerDense', 0x40000),
        ('ffn', 'PeerFfn', 0x60000), ('service', 'PeerService', 0x68000), ('affine', 'PeerAffine', 0x70b00),
        ('text', 'PeerText', 0x72000), ('gdn', 'PeerGdn', 0xb8000))]
    lines = ['/* SPDX-License-Identifier: MIT */',
             '// Mechanically generated from COMPLETE fixed FP32 persistent-token/QGW1 artifacts.',
             '// IMMUTABLE DATA ONLY. No native owner,source/init,journal,RUN/STOP or cleanup grant.',
             '#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE)',
             '#error Fixed QTP1/QGW1 program DATA cannot select the distinct precise ROOT/worker family',
             '#endif',
             '#ifndef PIONEER_Q38_TOKEN_PROGRAMS_H', '#define PIONEER_Q38_TOKEN_PROGRAMS_H',
             '#include "PioneerQ38TokenFootprint.h"', 'namespace PioneerQ38TokenPrograms {',
             f'enum {{Family=0x31505451,Version=1,CancelBoundary={int(cancel)}}};']
    for data_set, name, label, address in units:
        data = data_set[name+'.bin']
        require(data and not len(data)%4 and not address%4 and address+len(data) <= 0x180000, f'{label}: extent')
        lines += ['// SHA256 '+hashlib.sha256(data).hexdigest(), f'static const uint32_t {label}[]={{']
        words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
        lines += [' '+','.join(f'0x{word:08x}U' for word in words[i:i+8])+',' for i in range(0, len(words), 8)]
        lines += ['};', f'static_assert(sizeof({label})=={len(data)},"exact complete {label} DATA");']
    lines += ['using PioneerQ38TokenL1::Span;',
              'static inline unsigned CodeSpans(unsigned worker){return worker>=120?0:worker?8:4;}',
              'static inline Span CodeSpan(unsigned worker,unsigned span){',
              ' if(worker>=120 || span>=CodeSpans(worker))return {UINT32_MAX,0};', ' if(!worker)switch(span){']
    for i, (_, _, label, address) in enumerate(units[:4]):
        lines += [f'  case {i}:return {{0x{address:x},unsigned(sizeof({label})/4)}};']
    lines += [' }else switch(span){']
    for i, (_, _, label, address) in enumerate(units[4:]):
        lines += [f'  case {i}:return {{0x{address:x},unsigned(sizeof({label})/4)}};']
    lines += [' }return {UINT32_MAX,0};', '}',
              'static inline unsigned CodeWords(unsigned worker){',
              ' unsigned words=0;for(unsigned i=0;i<CodeSpans(worker);i++)words+=CodeSpan(worker,i).words;return words;', '}',
              'static inline uint32_t CodeAddress(unsigned worker,unsigned ordinal){',
              ' if(worker>=120)return UINT32_MAX;',
              ' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
              '  if(ordinal<s.words)return s.address+ordinal*4;', '  ordinal-=s.words;', ' }return UINT32_MAX;', '}',
              'static inline bool ProgramWord(unsigned worker,uint32_t address,uint32_t& output){',
              ' if(worker>=120 || (address&3) || (reinterpret_cast<uintptr_t>(&output)&3))return false;',
              ' const uintptr_t target=reinterpret_cast<uintptr_t>(&output);']
    for _, _, label, _ in units:
        lines += [f' if(target>=reinterpret_cast<uintptr_t>({label}) && target-reinterpret_cast<uintptr_t>({label})<sizeof({label}))return false;']
    lines += [' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
              '  if(address<s.address || address-s.address>=s.words*4)continue;',
              '  const uint32_t* data=nullptr;', '  if(!worker)switch(i){']
    for i, (_, _, label, _) in enumerate(units[:4]):
        lines += [f'   case {i}:data={label};break;']
    lines += ['  }else switch(i){']
    for i, (_, _, label, _) in enumerate(units[4:]):
        lines += [f'   case {i}:data={label};break;']
    lines += ['  }', '  if(!data)return false;', '  output=data[(address-s.address)/4];return true;',
              ' }return false; // no implicit zero-padded holes or runtime-data permission', '}', '}', '#endif', '']
    # All artifacts,including BOTH complete ELFs,were authenticated BEFORE
    # creating any output. Never replace or repair an existing header.
    encoded = '\n'.join(lines).encode()
    with output.open('xb') as target:
        require(target.write(encoded) == len(encoded), 'complete generated output write')
    print(hashlib.sha256(encoded).hexdigest()+f' token-programs FP32/cancel{int(cancel)}/ALL12 sections+2fullELFs authenticated/DATA ONLY/NO install or native permission')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print(f'PROGRAM_DATA_REFUSAL {error}', file=sys.stderr)
        sys.exit(2)
