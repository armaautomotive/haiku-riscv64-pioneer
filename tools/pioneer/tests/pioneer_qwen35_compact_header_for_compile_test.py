#!/usr/bin/env python3
"""Check compact compile headers cannot admit a simulator or loadable build."""
import os
from pathlib import Path
import subprocess
import tempfile

from pioneer_qwen35_compact_header_for_compile import generate

markers = ('profiled-compile-only.marker', 'compact-two-region.marker', 'activation-sharing.marker',
           'delta-shard.marker', 'flash-hybrid.marker', 'ffn-pair.marker', 'fused-rms-save.marker')
required = ('PIONEER_QWEN35_COMPACT_DRIVER_COMPILE_ONLY', 'PIONEER_QWEN35_DATAFLOW_REWRITE',
            'PIONEER_QWEN35_DEVICE_DISPATCH', 'PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS',
            'PIONEER_QWEN35_RECOVERY_COMPILE_ONLY', 'PIONEER_QWEN35_SNAPSHOT_TAKEOVER_WITNESS',
            'PIONEER_QWEN35_SHARED_ROLE_OWNER_WITNESS', 'PIONEER_QWEN35_SHARED_ROLE_GRAPH_WITNESS',
            'PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS', 'PIONEER_QWEN35_PAIRED_RECURRENCE',
            'PIONEER_QWEN35_FUSED_RECURRENCE', 'PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY',
            'PIONEER_QWEN35_FLASH_DECODE_COMPILE_ONLY', 'PIONEER_QWEN35_FFN_PAIR_COMPILE_ONLY',
            'PIONEER_QWEN35_FUSED_RMS_SAVE_COMPILE_ONLY', 'PIONEER_QWEN35_TREE_SETTLEMENT')
forbidden = ('PIONEER_QWEN35_RECOVERY_QUALIFY', 'PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL',
             'PIONEER_QWEN35_TTSIM_UNPROFILED_COMPILE_ONLY', 'PIONEER_QWEN35_DENSE_SHARD_COMPILE_ONLY')
symbols = ('00072000 T qwen35_dataflow_entry\n00072004 T qwen35_resident_graph_stopped_park\n'
           '00072008 T qwen35_resident_graph_service_main\n0007200c T qwen35_shared_compute_coordinate\n'
           '000a0400 T qwen35_graph_execute\n000a0404 T qwen35_graph_run_descriptor\n')


def must_reject(directory):
    try:
        generate(directory)
    except ValueError:
        return
    raise AssertionError('unsafe compact image accepted')


with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    image = root / 'image'; image.mkdir()
    for marker in markers:
        (image / marker).touch()
    (image / 'boot.bin').write_bytes(bytes.fromhex('6fc00600'))
    text = bytes.fromhex('130000006f0000001300000013000000')
    (image / 'text.bin').write_bytes(text)
    (image / 'dispatch.bin').write_bytes(bytes.fromhex('1300000013000000'))
    (image / 'symbols.txt').write_text(symbols, encoding='ascii')
    header = root / 'image.h'; header.write_text(generate(image), encoding='ascii')
    source = root / 'test.cpp'
    source.write_text('#include "image.h"\nstatic_assert(PioneerQwen35::DataflowServiceTextWords[1]==0x0000006fU);\n', encoding='ascii')
    compiler = [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only', str(source)]
    flags = [f'-D{name}' for name in required]
    assert subprocess.run(compiler + flags, capture_output=True).returncode == 0
    for missing in required:
        assert subprocess.run(compiler + [flag for flag in flags if flag != f'-D{missing}'],
                              capture_output=True).returncode != 0, missing
    for flag in forbidden:
        assert subprocess.run(compiler + flags + [f'-D{flag}'], capture_output=True).returncode != 0, flag
    for marker in markers:
        (image / marker).unlink(); must_reject(image); (image / marker).touch()
    for name in ('ttsim-unprofiled.marker', 'dense-shard.marker', 'flash.bin'):
        (image / name).touch(); must_reject(image); (image / name).unlink()
    (image / 'dispatch.bin').write_bytes(bytes(0x9c00))
    generate(image)
    (image / 'dispatch.bin').write_bytes(bytes(0x9c04))
    must_reject(image)
    (image / 'dispatch.bin').write_bytes(bytes(8))
    (image / 'text.bin').write_bytes(bytes(len(text)))
    must_reject(image)
    (image / 'text.bin').write_bytes(text)
    (image / 'symbols.txt').write_text(symbols + '00072000 T qwen35_dataflow_entry\n', encoding='ascii')
    must_reject(image)
print('PASS compact compile header guards, image boundaries and simulator rejection; NO HARDWARE')
