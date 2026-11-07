#!/usr/bin/env python3
"""Generate exact diagnostic code DATA; never install or execute firmware."""
import hashlib
import os
from pathlib import Path
import re
import sys

PINS = {
    'boot.bin': (4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
    'text.bin': (130580, '0b118528adadb2ab5be46c8d263ebbc9ff3dab02488efea9c9e3b437a6125b34'),
    'terminal.bin': (8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
    'conv.bin': (32216, 'b76f012be20af78ae5d7a3b027bf22e8dbab56ae2f7f0182f669832c2fc4de59'),
    'fast_decode.bin': (1496, 'a37bab87fc21f140236413c673352966bf9452212bf89e7956868c1cea15f9d1'),
    'program.elf': (193672, '882d2e1cf08ad5ccbd69c52a7ecb7daf5e7101527f669431faf962f158b5b425'),
}
assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
directory, output = map(Path, sys.argv[1:])
assert directory.is_absolute() and directory.is_dir()
assert output.is_absolute() and not output.exists() and not output.is_symlink()
here = Path(__file__).resolve().parent
source = (here / 'PioneerQ38FullOffloadVectorQ4Programs.h').read_bytes()
assert hashlib.sha256(source).hexdigest() == '3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
images = {}
for name, (size, digest) in PINS.items():
    path = directory / name
    assert path.is_file() and not path.is_symlink()
    data = path.read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
    images[name] = data

def array(label, data):
    assert data and len(data) % 4 == 0
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    result = '// SHA256 ' + hashlib.sha256(data).hexdigest() + '\nstatic const uint32_t ' + label + '[]={\n'
    result += '\n'.join(' ' + ','.join('0x%08xU' % w for w in words[i:i+8]) + ',' for i in range(0, len(words), 8))
    return result + '\n};\nstatic_assert(sizeof(' + label + ')==' + str(len(data)) + ',"exact complete ' + label + ' DATA");'

text = source.decode('ascii')
for label, name in {'RootBoot': 'boot.bin', 'RootText': 'text.bin', 'RootTerminal': 'terminal.bin', 'RootAux': 'conv.bin'}.items():
    pattern = r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ' + label + r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\(' + label + r'\)==[0-9]+,"exact complete ' + label + r' DATA"\);'
    text, count = re.subn(pattern, lambda _: array(label, images[name]), text, flags=re.S)
    assert count == 1
text = text.replace('PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4_PROGRAMS_H', 'PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROGRAMS_H')
text = text.replace('PioneerQ38FullOffloadVectorQ4Programs', 'PioneerQ38FullOffloadFastDecodePrograms')
text = text.replace('Generated complete five-EOG ROOT and ten exact vector-Q4 returning peer sections.',
                    'Generated generation-only timer ROOT; all ten vector92 peer sections unchanged.')
marker = 'using PioneerQ38TokenPushedL1::Span;'
assert text.count(marker) == 1
text = text.replace(marker, array('RootFastDecode', images['fast_decode.bin']) + '\n' + marker)
marker = 'enum {Family=0x33505451,Version=1,CancelBoundary=1,EogCount=5};'
assert text.count(marker) == 1
text = text.replace(marker, marker + '\nenum {FastDecodeProfile=1,TokenPhaseProfile=1,PhaseProfileVersion=3,PhaseProfileAddress=0x75710,PhaseProfileWords=60,RootSections=5};')
assert text.count('worker?10:4;') == 1
text = text.replace('worker?10:4;', 'worker?10:5;')
marker = '  case 3:return {0x64000,unsigned(sizeof(RootAux)/4)};'
assert text.count(marker) == 1
text = text.replace(marker, marker + '\n  case 4:return {0x6c000,unsigned(sizeof(RootFastDecode)/4)};')
marker = ' if(target>=reinterpret_cast<uintptr_t>(RootAux) && target-reinterpret_cast<uintptr_t>(RootAux)<sizeof(RootAux))return false;'
assert text.count(marker) == 1
text = text.replace(marker, marker + '\n if(target>=reinterpret_cast<uintptr_t>(RootFastDecode) && target-reinterpret_cast<uintptr_t>(RootFastDecode)<sizeof(RootFastDecode))return false;')
marker = '   case 3:data=RootAux;break;'
assert text.count(marker) == 1
text = text.replace(marker, marker + '\n   case 4:data=RootFastDecode;break;')
peers = lambda s: re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t Peer[A-Za-z]+\[\]=\{.*?\n\};\nstatic_assert\(sizeof\(Peer[A-Za-z]+\)==[0-9]+,"exact complete Peer[A-Za-z]+ DATA"\);', s, re.S)
assert len(peers(text)) == 10 and peers(text) == peers(source.decode('ascii'))
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ', text)) == 15
with output.open('x', encoding='ascii') as stream:
    stream.write(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(), output.name)
print('DATA_ONLY distinct full-card15 sections; exact new1496B code view; ten vector92 peers unchanged; NO install/run/speed result')
