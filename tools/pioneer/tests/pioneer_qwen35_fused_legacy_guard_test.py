#!/usr/bin/env python3
"""Check that the fused resident build rejects host-per-operation sequences."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
source = (root / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
names = ('Qwen35DataflowFfnSequence', 'Qwen35DataflowRecurrentLayer',
         'Qwen35DataflowDenseLayer', 'Qwen35DataflowDecoderSequence')
functions = []
for name in names:
    signature = f'static status_t\n{name}(PioneerProbeCookie* cookie,void* data,size_t length)\n'
    assert source.count(signature) == 1
    start = source.index(signature)
    opening = source.index('{', start + len(signature))
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == '{':
            depth += 1
        elif source[index] == '}':
            depth -= 1
            if depth == 0:
                functions.append(source[start:index + 1])
                break
    else:
        raise AssertionError(f'unclosed function {name}')

program = ('#include <cassert>\n#include <cstddef>\n'
           'using status_t=int; struct PioneerProbeCookie {}; constexpr int B_NOT_ALLOWED=-17;\n'
           + '\n'.join(functions) + '\nint main() {\n'
           + ''.join(f'assert({name}(nullptr,nullptr,0)==B_NOT_ALLOWED);\n' for name in names)
           + '}\n')
with tempfile.TemporaryDirectory() as temporary:
    test = Path(temporary) / 'test.cpp'
    binary = Path(temporary) / 'test'
    test.write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-DPIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY', str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS fused resident build rejects host-per-operation sequences; NO HARDWARE')
