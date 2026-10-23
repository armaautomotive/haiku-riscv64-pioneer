#!/usr/bin/env python3
"""Evaluate actual DEBUG guards/builders;no SDK/device/native job required."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

here = pathlib.Path(__file__).resolve().parent
repo = here.parents[2]
tools = here.parent
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0
with tempfile.TemporaryDirectory(prefix='pioneer-rms-debug-gates-') as temporary:
    root = pathlib.Path(temporary)
    missing, output = root / 'MUST_NOT_READ_SDK', root / 'MUST_NOT_CREATE_OUTPUT'
    for name in ('banks_driver', 'banks_client', 'inference', 'local_image', 'ffn_graph_image',
                 'ffn_matrix_image', 'ffn_persistent_image', 'projection_matrix_image',
                 'projection_persistent_image', 'projection_graph_image'):
        command = ['sh', str(tools / ('build_qwen38_' + name + '.sh')), str(missing), str(output)]
        for flag in ('PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT'):
            for value in ('1', '2', '-1', '', '00', 'false'):
                result = subprocess.run(command, env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists(), (name, flag, value)
                count += 1
        result = subprocess.run(command, env={**clean, 'PIONEER_DRIVER_BUILD_TAG': '2026102200010041'},
                                capture_output=True, text=True, timeout=5)
        assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
        count += 1
    for command in ([sys.executable, str(tools / 'build_qwen38_embedding_native.py'), str(missing), str(output), '--rms-validation'],
                    ['sh', str(tools / 'build_qwen38_embedding_client.sh'), str(missing), str(output), '--rms-validation']):
        for flag in ('PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT', 'PIONEER_UNKNOWN_ROLE'):
            for value in ('1', '2', '-1', '', '00', 'false'):
                result = subprocess.run(command, env={**clean, flag: value}, capture_output=True, text=True, timeout=5)
                assert result.returncode == 2 and not result.stdout and not result.stderr and not output.exists()
                count += 1

    driver = (repo / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
    # Only strip includes from the ACTUAL pre-SDK feature guard prefix. The
    # full real SDK build separately compiles/includes/links the actual code.
    prefix = driver.split('\n#ifdef PIONEER_Q38_EMBEDDING_NATIVE\n', 1)[0]
    prefix = re.sub(r'^#include .*$', '', prefix, flags=re.MULTILINE)
    flags = {'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE': '1', 'PIONEER_Q38_LEARNED_RMS_NATIVE': '1',
             'PIONEER_Q38_EMBEDDING_NATIVE': '1', 'PIONEER_Q38_EMBEDDING_QUALIFICATION': '1',
             'TT_PIONEER_DRIVER_RELOAD': '1', 'PIONEER_DRIVER_BUILD_TAG': '0x2026102200010041ULL'}

    def preprocess(text, definitions, okay):
        global count
        result = subprocess.run([os.environ.get('CXX', 'c++'), '-x', 'c++', '-E',
                                 *['-D' + k + '=' + v for k, v in definitions.items()], '-'],
                                input=text, env=clean, capture_output=True, text=True, timeout=5)
        assert (result.returncode == 0) == okay, (definitions, result.stderr)
        count += 1

    preprocess(prefix, flags, True)
    for flag in flags:
        changed = dict(flags)
        del changed[flag]
        preprocess(prefix, changed, False)
    for flag in tuple(flags)[:4]:
        for value in ('0', '2', '-1'):
            preprocess(prefix, {**flags, flag: value}, False)
    for tag in ('0x2026102200010039ULL', '0x2026102200010040ULL', '0x2026102200010038ULL', '0x2026102100010012ULL'):
        preprocess(prefix, {**flags, 'PIONEER_DRIVER_BUILD_TAG': tag}, False)
    for flag in ('PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
                 'PIONEER_Q38_EMBEDDING_VALIDATION_NATIVE', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER',
                 'PIONEER_Q38_FFN_HOST_PROFILE', 'PIONEER_Q38_FFN_SEALED_PROOF'):
        preprocess(prefix, {**flags, flag: '1'}, False)
    client = (tools / 'PioneerQ38LearnedRmsSession.h').read_text().split('#include', 1)[0] + '#endif\n'
    refs = {'PIONEER_Q38_LEARNED_RMS_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1',
            'PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE': '1'}
    preprocess(client, refs, True)
    for flag in refs:
        changed = dict(refs)
        del changed[flag]
        preprocess(client, changed, False)
        for value in ('0', '2', '-1'):
            preprocess(client, {**refs, flag: value}, False)
    for flag in ('PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT', 'PIONEER_Q38_EMBEDDING_CLIENT',
                 'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_LEARNED_RMS_VALIDATION_NATIVE',
                 'PIONEER_Q38_EMBEDDING_NATIVE', '_KERNEL_MODE'):
        preprocess(client, {**refs, flag: '1'}, False)
    # Explicitly retain old embedding-only reference permission and deny
    # combined role aliases/Work/kernel,using the actual shared header guard.
    reference = (tools / 'PioneerQ38EmbeddingValidationReference.h').read_text().split('#include', 1)[0] + '#endif\n'
    preprocess(reference, refs, True)
    old = {'PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT': '1', 'PIONEER_Q38_CPU_VALIDATION': '1'}
    preprocess(reference, old, True)
    for extra in ({'PIONEER_Q38_EMBEDDING_VALIDATION_CLIENT': '1'}, {'PIONEER_Q38_WORK_INFERENCE': '1'}, {'_KERNEL_MODE': '1'}):
        preprocess(reference, {**refs, **extra}, False)
print(f'PASS RMS DEBUG isolation gates{count};actual driver/client/reference guards;reservedtag41/namespace12;old/new builders refuse before SDK/output;NO device access')
