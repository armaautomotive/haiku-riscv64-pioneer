#!/usr/bin/env python3
"""New complete full-card firmware DATA: five-EOG ROOT + exact wide peers.

No model weights, accelerator access, installation or execution. Four ROOT
sections and the whole ELF are authenticated together; ten peer sections
remain the byte-exact previously qualified returning-wide package.
"""
import hashlib
import os
import pathlib
import re
import sys
from generate_qwen38_token_programs import authenticated, pins

assert __debug__ and len(sys.argv) == 3
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
directory, output = map(pathlib.Path, sys.argv[1:])
assert directory.is_absolute() and output.is_absolute() and not output.exists() and not output.is_symlink()
here = pathlib.Path(__file__).resolve().parent
root = authenticated(directory, pins(here / 'build_qwen38_full_offload_batched_root.py', 'PINS'))
source = (here / 'PioneerQ38TokenWideGdnPrograms.h').read_bytes()
assert hashlib.sha256(source).hexdigest() == 'd285c9adb421fc0010d8ad48371eb58ca1480fc0d787f7c147afd4a93457a787'
text = source.decode('ascii')
start = text.index('#ifndef PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS_H')
text = text[start:]
for label, name in (('RootBoot', 'boot'), ('RootText', 'text'), ('RootTerminal', 'terminal'), ('RootAux', 'conv')):
    data = root[name + '.bin']
    digest = hashlib.sha256(data).hexdigest()
    words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
    array = '// SHA256 ' + digest + '\nstatic const uint32_t ' + label + '[]={\n'
    array += '\n'.join(' ' + ','.join('0x%08xU' % w for w in words[i:i+8]) + ',' for i in range(0, len(words), 8))
    array += '\n};\nstatic_assert(sizeof(' + label + ')==' + str(len(data)) + ',"exact complete ' + label + ' DATA");'
    pattern = r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ' + label + r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\(' + label + r'\)==[0-9]+,"exact complete ' + label + r' DATA"\);'
    text, count = re.subn(pattern, lambda _: array, text, flags=re.S)
    assert count == 1
text = text.replace('PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS_H', 'PIONEER_Q38_FULL_OFFLOAD_BATCHED_PROGRAMS_H')
text = text.replace('PioneerQ38TokenWideGdnPrograms', 'PioneerQ38FullOffloadBatchedPrograms')
assert text.count('Family=0x33505451,Version=1,CancelBoundary=1') == 1
text = text.replace('Family=0x33505451,Version=1,CancelBoundary=1',
                    'Family=0x33505451,Version=1,CancelBoundary=1,EogCount=5')
text = ('/* SPDX-License-Identifier: MIT */\n'
        '// Generated complete five-EOG ROOT and ten byte-exact returning-wide peer sections.\n'
        '// Executable code DATA only;not model weights or physical/performance authority.\n'
        '#include "PioneerQ38FullOffloadBatchedProgramsRole.h"\n' + text)
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ', text)) == 14
with output.open('x', encoding='ascii') as stream:
    stream.write(text)
print(hashlib.sha256(output.read_bytes()).hexdigest(), output.name)
print('PASS generated complete full-card batched14-section DATA;all5 EOG ROOT/ten exact wide peers;NO learned weights or hardware')
