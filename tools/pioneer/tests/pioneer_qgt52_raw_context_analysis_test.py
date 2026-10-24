#!/usr/bin/env python3
"""Synthetic parser/arithmetic checks,not physical model qualification."""
import importlib.util
import pathlib
import json
import subprocess
import sys
import tempfile
import unittest

path = pathlib.Path(__file__).resolve().parents[1]/'analyze_qgt52_raw_context.py'
spec = importlib.util.spec_from_file_location('raw_context', path)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


def fixture(head=47, lane=49, expected_raw=1.0, actual_raw=1.0, expected=0.25, actual=0.25):
    half = head*2+lane//64
    offset = 8578+lane % 64
    result = [f'DEBUG_CPU_ONLY_FAILURE reasonrelative_tolerance op000089de word{half*8642+offset} half{half} offset{offset} expected_bits{probe.bits(expected):08x} actual_bits{probe.bits(actual):08x} reply_captured1;NO_ADDITIONAL_DEVICE_IO',
              f'DEBUG_CPU_GATED_CONTEXT head{head} lane{lane};PAIRED_RAW_FROM_EXISTING_REPLY_ORIGINAL_Z_FROM_PREPARED_REFERENCE_NO_ADDITIONAL_DEVICE_IO']
    for name, value in [('EXPECTED_RAW', expected_raw), ('ACTUAL_RAW', actual_raw), ('ORIGINAL_Z', -1.0)]:
        for at in range(0, 128, 8):
            result.append(f'DEBUG_CPU_{name} word{at:03d}'+f' {probe.bits(value):08x}'*8)
    return '\n'.join(result)


class ContextTests(unittest.TestCase):
    def test_exact_and_boundaries(self):
        for head in (0, 47):
            for lane in (0, 49, 63, 64, 127):
                result = probe.analyze(probe.parse(fixture(head, lane)))
                self.assertEqual((result['head'], result['lane']), (head, lane))
                self.assertEqual(result['raw_rms_ratio'], 1.0)
                self.assertEqual(result['residual_relative_error_to_observed'], 0.0)
                self.assertFalse(result['log_authenticated'])

    def test_uniform_scale_cancel_and_sign(self):
        for scale in (2.0, -2.0):
            result = probe.analyze(probe.parse(fixture(actual_raw=scale, actual=0.25 if scale > 0 else -0.25)))
            self.assertLess(abs(abs(result['raw_rms_ratio'])-1), 1e-6)
            self.assertLess(result['residual_relative_error_to_observed'], 1e-6)

    def test_zero_cannot_attribute(self):
        result = probe.analyze(probe.parse(fixture(expected_raw=0, actual_raw=0)))
        self.assertFalse(result['estimate_available'])

    def test_reject_bad_records(self):
        text = fixture()
        bad = [text+'\n'+text, text.replace('head47', 'head48'), text.replace('lane49', 'lane128'),
               text.replace('half94', 'half95'), text.replace('offset8627', 'offset8626'),
               text.replace('op000089de', 'op00000000'), text.replace('reply_captured1', 'reply_captured0'),
               text.replace('reasonrelative_tolerance', 'reasontransport'), text.replace('word000', 'word001', 1),
               text.replace(' 3f800000', ' invalid', 1), text.replace('word120', 'word128', 1)]
        rows = text.splitlines()
        bad += ['\n'.join(rows[:i]+rows[i+1:]) for i in range(len(rows))]
        bad += [text+'\n'+row for row in rows[2:]]
        bad.append(text.replace('head47', 'head'+'9'*10000))
        for data in bad:
            with self.assertRaises(ValueError):
                probe.parse(data)

    def test_reject_nonfinite_subnormal_or_outside_domain(self):
        for word in (1, 0x807fffff, 0x47800000, 0x7f800000, 0x7fc00000):
            for name in ('EXPECTED_RAW', 'ACTUAL_RAW', 'ORIGINAL_Z'):
                context = probe.parse(fixture())
                context[name][127] = word
                with self.assertRaises(ValueError):
                    probe.analyze(context)
        for value in (float('inf'), float('nan')):
            with self.assertRaises(ValueError):
                probe.analyze(probe.parse(fixture(actual=value)))

    def test_read_only_cli_and_bounded_input(self):
        with tempfile.TemporaryDirectory(dir='/private/tmp', prefix='pioneer-qgt-analysis-') as temporary:
            log = pathlib.Path(temporary)/'context.log'
            for content, status in [(fixture().encode(), 0), (b'no context', 2), (b'\xff', 2), (b'x'*(1024*1024+1), 2)]:
                log.write_bytes(content)
                result = subprocess.run([sys.executable, str(path), str(log)], capture_output=True, text=True, timeout=5)
                self.assertEqual(result.returncode, status)
                self.assertEqual(log.read_bytes(), content)
                if not status:
                    self.assertFalse(json.loads(result.stdout)['log_authenticated'])
                    self.assertFalse(result.stderr)
                else:
                    self.assertFalse(result.stdout)
                    self.assertTrue(result.stderr.startswith('STOP_CPU_CONTEXT_ANALYSIS:'))


if __name__ == '__main__':
    unittest.main()
