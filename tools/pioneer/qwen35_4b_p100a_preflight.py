#!/usr/bin/env python3
"""Authenticate and inventory the pinned Qwen3.5-4B text model (no device I/O)."""
from collections import Counter, defaultdict
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import sys

from tenstorrent_gguf_preflight import FORMATS, MAX_HEADER, inspect

MODEL_BYTES = 2_740_937_888
MODEL_SHA = '00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4'
REVISION = 'e87f176479d0855a907a41277aca2f8ee7a09523'
EXPECTED_FORMATS = {'F32': 177, 'Q4_K': 131, 'Q5_K': 48, 'Q6_K': 22, 'Q8_0': 48}
BLOCK = {value[0]: value[1:] for value in FORMATS.values()}
EXPECTED_METADATA = {'general.architecture': 'qwen35', 'general.name': 'Qwen3.5-4B',
                     'general.file_type': 15, **{'qwen35.' + key: value for key, value in {
                         'block_count': 32, 'full_attention_interval': 4, 'context_length': 262144,
                         'embedding_length': 2560, 'feed_forward_length': 9216,
                         'attention.head_count': 16, 'attention.head_count_kv': 4,
                         'attention.key_length': 256, 'attention.value_length': 256,
                         'ssm.conv_kernel': 4, 'ssm.state_size': 128, 'ssm.group_count': 16,
                         'ssm.time_step_rank': 32, 'ssm.inner_size': 4096}.items()}}


def entries():
    common = {'attn_norm.weight': ((2560,), 'F32'),
              'post_attention_norm.weight': ((2560,), 'F32'),
              'ffn_gate.weight': ((2560, 9216), 'Q4_K'),
              'ffn_up.weight': ((2560, 9216), 'Q4_K'),
              'ffn_down.weight': ((9216, 2560), 'Q4_K Q6_K')}
    full = {'attn_q.weight': ((2560, 8192), 'Q4_K'),
            'attn_k.weight': ((2560, 1024), 'Q4_K'),
            'attn_v.weight': ((2560, 1024), 'Q4_K Q6_K'),
            'attn_output.weight': ((4096, 2560), 'Q4_K'),
            'attn_q_norm.weight': ((256,), 'F32'), 'attn_k_norm.weight': ((256,), 'F32')}
    recurrent = {'attn_gate.weight': ((2560, 4096), 'Q4_K'),
                 'attn_qkv.weight': ((2560, 8192), 'Q5_K'),
                 'ssm_out.weight': ((4096, 2560), 'Q5_K'),
                 'ssm_conv1d.weight': ((4, 8192), 'F32'),
                 'ssm_a': ((32,), 'F32'), 'ssm_dt.bias': ((32,), 'F32'),
                 'ssm_norm.weight': ((128,), 'F32'),
                 'ssm_alpha.weight': ((2560, 32), 'Q8_0'), 'ssm_beta.weight': ((2560, 32), 'Q8_0')}
    expected = {'output_norm.weight': ((2560,), 'F32'),
                'token_embd.weight': ((2560, 248320), 'Q6_K')}
    for layer in range(32):
        expected.update({f'blk.{layer}.{name}': spec
                         for name, spec in (common | (full if layer % 4 == 3 else recurrent)).items()})
    return expected


def inventory(parsed):
    expected = entries()
    meta = parsed['metadata']
    if any(meta.get(k) != v for k, v in EXPECTED_METADATA.items()):
        raise ValueError('unexpected 4B metadata')
    tensors = parsed['tensors']
    if len(tensors) != len(expected) or set(expected) != {t['name'] for t in tensors}:
        raise ValueError('missing or extra 4B tensor, or unexpected untied head')
    if Counter(t['format'] for t in tensors) != EXPECTED_FORMATS:
        raise ValueError('unexpected 4B format counts')
    tiled = defaultdict(int)
    shapes = Counter()
    for t in tensors:
        name, fmt, shape = t['name'], t['format'], tuple(t['shape'])
        dims, formats = expected[name]
        if shape != dims or fmt not in formats.split():
            raise ValueError(f'unexpected 4B tensor shape or format for {name}')
        if fmt == 'F32':
            continue
        width, rows = shape
        elements, block_bytes = BLOCK[fmt]
        # Layout estimate only. Tied embedding is streamed as the readout
        # head once per token; only the actual embedding input is a row lookup.
        tiled[fmt] += width // elements * (rows // 16) * ((16 * block_bytes + 63) // 64 * 64)
        shapes[(width, rows, fmt)] += 1
    return {'layers': 32, 'recurrent_layers': 24, 'full_attention_layers': 8,
            'tied_output_head': True, 'tensor_count': len(tensors), 'formats': EXPECTED_FORMATS,
            'quantized_matrix_shapes': [{'input': k, 'output': n, 'format': fmt, 'count': count}
                                       for (k, n, fmt), count in sorted(shapes.items())],
            'estimated_tiled_quantized_bytes_per_decode_pass': dict(sorted(tiled.items())),
            'bytes_are_layout_estimates_not_runtime_counters': True,
            'runtime_gap': {'new_input_widths': sorted({k for k, _, _ in shapes}),
                            'unhandled_source_format': 'Q5_K',
                            'needs_separate_card_layout_and_tied_head_binding': True}}


def identity(s):
    return s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns


def verify(path):
    with os.fdopen(os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK), 'rb') as f:
        before = os.fstat(f.fileno())
        if not stat.S_ISREG(before.st_mode) or before.st_size != MODEL_BYTES:
            raise ValueError('expected exact pinned 4B file size')
        digest = hashlib.sha256()
        observed = 0
        while data := f.read(8 * 1024 * 1024):
            observed += len(data)
            if observed > MODEL_BYTES:
                raise ValueError('4B file extended while reading')
            digest.update(data)
        if observed != MODEL_BYTES or digest.hexdigest() != MODEL_SHA:
            raise ValueError('full 4B SHA-256 mismatch')
        f.seek(0)
        layout = inventory(inspect(f.read(MAX_HEADER), before.st_size, include_tensors=True))
        if identity(os.fstat(f.fileno())) != identity(before):
            raise ValueError('4B source changed during inspection')
    return {'revision': REVISION, 'model_sha256': MODEL_SHA, 'model_bytes': MODEL_BYTES,
            'scope': 'same-open full-file authentication and host layout only; no inference', **layout}


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('use PINNED_QWEN35_4B_GGUF')
    try:
        print(json.dumps(verify(Path(sys.argv[1])), sort_keys=True))
    except (OSError, ValueError, struct.error) as error:
        raise SystemExit(f'REFUSE_QWEN35_4B_PREFLIGHT {error}')
