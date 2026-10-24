#!/usr/bin/env python3
"""Actual role/build/whole-image/frame gates. No Pioneer access."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
prefix = (here / 'blackhole_qwen38_gdn_output_graph.cpp').read_text().split('#define PIONEER_ATTENTION_FREESTANDING', 1)[0]
cases = 0

def pre(flags, valid):
    global cases
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-E', '-x', 'c++', *['-D'+k+'='+v for k, v in flags.items()], '-'],
                            input=prefix, capture_output=True, text=True, env=clean, timeout=5)
    assert (result.returncode == 0) == valid, (flags, result.stderr)
    cases += 1

role = 'PIONEER_Q38_GDN_OUTPUT_GRAPH_ROOT'
pre({role: '1'}, True)
for value in ('0', '2', '-1', 'bad'):
    pre({role: value}, False)
pre({}, False)
for key in ('_KERNEL_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_PROJECTION_RESULT_STASH',
            'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE', 'PIONEER_Q38_GDN_RENDEZVOUS_NATIVE', 'PIONEER_Q38_GDN_GATED_NATIVE',
            'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_STATE_INIT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
            'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
            'PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE', 'PIONEER_Q38_LEARNED_RMS_NATIVE'):
    for value in ('0', '1'):
        pre({role: '1', key: value}, False)

with tempfile.TemporaryDirectory(prefix='pioneer-gdn-output-graph-gates-') as temporary:
    root = pathlib.Path(temporary)
    for key in (role, 'PIONEER_Q38_GDN_OUTPUT_PRODUCER_NATIVE', 'PIONEER_Q38_PROJECTION_RESULT_STASH',
                'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '2', '-1', '', 'false', '00'):
            for builder, runner in (('build_qwen38_gdn_output_graph_image.sh', 'sh'), ('build_qwen38_gdn_output_bootstrap_native.py', sys.executable)):
                result = subprocess.run([runner, str(here / builder), str(root / 'NO_SDK'), str(root / 'NO_OUTPUT')],
                                        capture_output=True, text=True, env={**clean, key: value}, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not (root / 'NO_OUTPUT').exists()
                cases += 1
    commands = [s for s in (here / 'build_qwen38_gdn_output_graph_image.sh').read_text().splitlines() if s.startswith('awk ')]
    assert len(commands) == 1
    for legacy, new, valid in ((4976, 2048, True), (4977, 2048, False), (4975, 2048, False), (4976, 2049, False)):
        (root / 'program.su').write_text(f'actual:1:1:unsigned int q38_projection_matrix_once()\t{legacy}\tstatic\nactual:2:1:new_function\t{new}\tstatic\n')
        (root / 'gather.su').write_text('actual:3:1:gather\t880\tstatic\n')
        command = commands[0].replace('"$output/program.su"', '"'+str(root / 'program.su')+'"').replace('"$output/gather.su"', '"'+str(root / 'gather.su')+'"')
        result = subprocess.run(['sh', '-c', command], capture_output=True, env=clean, timeout=5)
        assert (result.returncode == 0) == valid
        cases += 1

assert len(sys.argv) == 4, 'usage: gates.py NEW REPEAT OLD'
pins = {'boot': 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b',
        'text': '2cca19748a0771f2ff6e5e08ea5c704ae43cdcb83771d0df94e104734c8e84b1',
        'prefix': '5332aeac39d34773812477bfa243402c1d3fbf4b372a73f0d8a16b2718009961',
        'graph': 'dc89a3f1e25ec58a5bf2e58d3dd84aa85e19546993f5cdda2ac3a7e4ce4592d2'}
old_pins = {'boot': pins['boot'], 'text': 'c623cad56856ac690a79e9b60566148afdba062646bafbb5bf32bcabaa7cb108',
            'prefix': 'cfb512c2b507d511b671bbb73101b724ec230f853a3486616336bacd66706357',
            'graph': '13992bf95da5c695aab493db6d2760d74b139ed7384c04552df0751fa13ceaf9'}
for path, hashes in ((pathlib.Path(sys.argv[1]), pins), (pathlib.Path(sys.argv[2]), pins), (pathlib.Path(sys.argv[3]), old_pins)):
    for section, digest in hashes.items():
        assert hashlib.sha256((path / (section + '.bin')).read_bytes()).hexdigest() == digest, (path, section)
    symbols = (path / 'program.symbols').read_text()
    assert not re.search(r'\b(?:U|[BbDdGgSs])\s', symbols)
    assert not re.search(r'\b(?:memcpy|memset|malloc|free|_GLOBAL_OFFSET_TABLE_)\b', symbols)
    assert not re.search(r'\b(?:divu?|remu?|mulh(?:u|su)?)\b', (path / 'program.disassembly').read_text())
    for name, pc in (('block_entry', 0x72000), ('block_park', 0x72038), ('block_fault_park', 0x72060)):
        assert re.search(r'^%08x\s+[Tt]\s+%s$' % (pc, name), symbols, re.M)
    for section, limit in (('text', 8192), ('prefix', 16384), ('graph', 24576)):
        assert 0 < (path / (section + '.bin')).stat().st_size <= limit
print(f'PASS QOP1 role/isolation/early-builder/frame cases{cases},repeat complete images/PC/no globals/division,EXACT old QPR1;NO native or hardware grant')
