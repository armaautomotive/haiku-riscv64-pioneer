#!/usr/bin/env python3
"""Real pinned GGUF plus negative layout/file identity checks, no card access."""
from copy import deepcopy
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import qwen35_4b_p100a_preflight as preflight


def refuse(call, expected):
    try:
        call()
    except (OSError, ValueError) as exc:
        assert expected in str(exc), exc
    else:
        raise AssertionError('invalid input accepted')


def run(path):
    report = preflight.verify(path)
    assert report['tensor_count'] == 426 and report['tied_output_head']
    assert report['formats']['Q5_K'] == 48
    assert report['runtime_gap']['new_input_widths'] == [2560, 4096, 9216]
    assert report['estimated_tiled_quantized_bytes_per_decode_pass']['Q6_K'] >= 521_472_000

    with path.open('rb') as f:
        parsed = preflight.inspect(f.read(preflight.MAX_HEADER), preflight.MODEL_BYTES, True)
    for operation, expected in (
        (lambda p: p['metadata'].update({'qwen35.full_attention_interval': 2}), 'metadata'),
        (lambda p: p['tensors'][0].update(shape=[5120]), 'shape or format'),
        (lambda p: p['tensors'][0].update(format='Q8_0'), 'format counts'),
        (lambda p: p['tensors'][0].update(name='output.weight'), 'untied head'),
        (lambda p: p['tensors'].pop(), 'missing or extra'),
        (lambda p: p['tensors'].append(p['tensors'][0]), 'missing or extra'),
    ):
        wrong = deepcopy(parsed)
        operation(wrong)
        refuse(lambda: preflight.inventory(wrong), expected)
    with tempfile.TemporaryDirectory() as tmp:
        wrong = Path(tmp) / 'wrong.gguf'
        wrong.write_bytes(b'wrong')
        refuse(lambda: preflight.verify(wrong), 'file size')
        with patch.object(preflight, 'MODEL_BYTES', 5):
            refuse(lambda: preflight.verify(wrong), 'SHA-256 mismatch')
        with patch.object(preflight, 'identity', side_effect=[1, 2]):
            refuse(lambda: preflight.verify(path), 'changed during inspection')
    print('PASS pinned 4B same-open full-file authentication and nine negative checks; no inference')


if __name__ == '__main__':
    if not __debug__ or len(sys.argv) != 2:
        raise SystemExit('use PINNED_QWEN35_4B_GGUF with assertions enabled')
    run(Path(sys.argv[1]))
