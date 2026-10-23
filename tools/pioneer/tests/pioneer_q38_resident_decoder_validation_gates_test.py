#!/usr/bin/env python3
"""Exact DEBUG44/reference roles;no SDK/device/model/library access."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0

def preprocess(source, flags, accepted):
    global count
    result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E',
                             *['-D' + k + '=' + v for k, v in flags.items()], '-'],
                            input=source, env=clean, capture_output=True, text=True, timeout=5)
    assert (result.returncode == 0) == accepted, (flags, result.stderr)
    count += 1

driver = (here.parents[2] / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
prefix = driver.split('\n#ifdef PIONEER_Q38_EMBEDDING_NATIVE\n', 1)[0]
prefix = re.sub(r'^#include .*$', '', prefix, flags=re.MULTILINE)
native = dict(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE='1', PIONEER_Q38_RESIDENT_DECODER_NATIVE='1',
              PIONEER_Q38_RESIDENT_DECODER_OWNER='1', PIONEER_Q38_DECODER_RUNTIME='1',
              PIONEER_Q38_PROJECTION_EXECUTION_OWNER='1', PIONEER_Q38_PROJECTION_RESULT_STASH='1',
              PIONEER_Q38_LEARNED_RMS_NATIVE='1', PIONEER_Q38_EMBEDDING_NATIVE='1',
              TT_PIONEER_DRIVER_RELOAD='1', PIONEER_DRIVER_BUILD_TAG='0x2026102300010044ULL',
              PIONEER_Q38_FFN_NATIVE_GRAPH='1', PIONEER_Q38_FFN_PERSISTENT_GRAPH='1',
              PIONEER_Q38_FFN_GRAPH_INPUT_ROOT='1', PIONEER_Q38_FFN_MODEL_INIT='1',
              PIONEER_Q38_GDN_MUTABLE_STATE='1', PIONEER_Q38_NORMAL_CLOCK='1',
              PIONEER_Q38_SPAN_PRELOAD='1', PIONEER_Q38_SPAN_FAST='1',
              PIONEER_Q38_LOCAL_DELTA_PROBE='1', TT_PIONEER_EXPERIMENTAL_Q38_STREAM='1')
preprocess(prefix, native, True)
for key in tuple(native)[:10]:
    for value in (None, '0', '2', '-1'):
        bad = dict(native)
        if value is None:
            del bad[key]
        else:
            bad[key] = value
        preprocess(prefix, bad, False)
for tag in ('0x2026102300010043ULL', '0x2026102200010042ULL', '0x2026102100010012ULL'):
    preprocess(prefix, {**native, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)

refs = dict(PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT='1', PIONEER_Q38_CPU_VALIDATION='1',
            PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE='1')
for name in ('PioneerQ38ResidentDecoderValidationSession.h', 'PioneerQ38ResidentDecoderValidationReference.h', 'PioneerQ38ProjectionValidationReference.h',
             'PioneerQ38EmbeddingValidationReference.h'):
    text = (here.parent / name).read_text().split('#include', 1)[0] + '#endif\n'
    preprocess(text, refs, True)
    required = tuple(refs) if 'ResidentDecoder' in name or 'Projection' in name else tuple(refs)[:2]
    for key in required:
        for value in (None, '0', '2', '-1'):
            bad = dict(refs)
            if value is None:
                del bad[key]
            else:
                bad[key] = value
            preprocess(text, bad, False)
    for flag in ('PIONEER_Q38_EMBEDDING_CLIENT', 'PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT',
                 'PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT', 'PIONEER_Q38_PROJECTION_VALIDATION_CLIENT',
                 'PIONEER_Q38_WORK_INFERENCE', '_KERNEL_MODE'):
        for value in ('0', '1'):
            preprocess(text, {**refs, flag: value}, False)
    if 'Embedding' not in name:
        for flag in ('PIONEER_Q38_NATIVE_Q8_LAYOUT', 'PIONEER_Q38_RESIDENT_DECODER_NATIVE',
                     'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_EMBEDDING_NATIVE'):
            for value in ('0', '1'):
                preprocess(text, {**refs, flag: value}, False)
with tempfile.TemporaryDirectory(prefix='pioneer-decoder-debug-build-gates-') as temporary:
    root = pathlib.Path(temporary)
    missing, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for command in ([sys.executable, str(here.parent / 'build_qwen38_resident_decoder_validation_native.py'), str(missing), str(output)],
                    ['sh', str(here.parent / 'build_qwen38_resident_decoder_validation_client.sh'), str(missing), str(output)]):
        for flag in ('PIONEER_Q38_RESIDENT_DECODER_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_DECODER_VALIDATION_CLIENT', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '-1', '', '00', 'false'):
                result = subprocess.run(command, env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1
print(f'PASS exact tag44 native and independent ORIGINAL reference guards{count};no old role alias/zero-valued mixed Work or native reference promotion;NO SDK/device/dirty-library access')
