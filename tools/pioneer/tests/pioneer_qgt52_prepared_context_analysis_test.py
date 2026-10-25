#!/usr/bin/env python3
"""Synthetic read-only prepared/NEXT-row parser and arithmetic tests."""
import json
import pathlib
import runpy
import subprocess
import sys
import tempfile
import unittest

path = pathlib.Path(__file__).resolve().parents[1]/'analyze_qgt52_prepared_context.py'
probe = runpy.run_path(str(path))
raw = probe['raw_tools']


def fixture(head=47, lane=49):
    half, offset = head*2+lane//64, 8578+lane % 64
    rows = [f'DEBUG_CPU_ONLY_FAILURE reasonrelative_tolerance op000089de word{half*8642+offset} half{half} offset{offset} expected_bits3e800000 actual_bits3e851eb8 reply_captured1;NO_ADDITIONAL_DEVICE_IO',
            f'DEBUG_CPU_GATED_CONTEXT head{head} lane{lane};PAIRED_RAW_FROM_EXISTING_REPLY_ORIGINAL_Z_FROM_PREPARED_REFERENCE_NO_ADDITIONAL_DEVICE_IO']
    bits, f32 = raw['bits'], raw['f32']
    data = {'ORIGINAL_Z': [bits(-1.0)]*128}
    for side, beta in (('EXPECTED', .5), ('ACTUAL', .625)):
        prepared = [0]*322
        prepared[0], prepared[128], prepared[256+lane % 64] = bits(.25), bits(.5), bits(.75)
        prepared[320], prepared[321] = bits(.5), bits(beta)
        state = f32(.5*f32(.75*beta))
        data[side+'_PREPARED_0'] = list(prepared)
        data[side+'_PREPARED_1'] = list(prepared)
        data[side+'_NEXT_ROW'] = [bits(state)]+[0]*127
        data[side+'_RAW'] = [0]*128
        data[side+'_RAW'][lane] = bits(f32(f32(state*.25)*f32(.0883883461356163)))
    for name in ('EXPECTED_RAW', 'ACTUAL_RAW', 'ORIGINAL_Z'):
        for at in range(0, 128, 8):
            rows.append(f'DEBUG_CPU_{name} word{at:03d}'+''.join(f' {v:08x}' for v in data[name][at:at+8]))
    rows.append(probe['MARKER'])
    for name in probe['SIZES']:
        for at in range(0, len(data[name]), 8):
            rows.append(f'DEBUG_CPU_{name} word{at:03d}'+''.join(f' {v:08x}' for v in data[name][at:at+8]))
    return '\n'.join(rows)


class PreparedContextTests(unittest.TestCase):
    def test_physical_prepared_capture_narrows_drift_before_state_update(self):
        captured = path.parent/'tests/data/qgt52-head47-prepared-context-20261024.log'
        text = captured.read_text()
        self.assertEqual(len(text.splitlines()), 247)
        context = probe['parse'](text)
        result = probe['analyze'](context)
        self.assertEqual((context['head'], context['lane'], context['word']), (47, 49, 820975))
        self.assertEqual((context['expected_bits'], context['actual_bits']), (0x3a86371e, 0x3a86373a))
        self.assertFalse(result['log_authenticated'])
        for side, word in (('expected', 'b69a7f68'), ('actual', 'b69a7f89')):
            self.assertEqual(result[side]['raw_from_prepared_bits'], word)
            self.assertEqual(result[side]['raw_from_next_row_bits'], word)
            self.assertEqual(result[side]['reported_raw_bits'], word)
            self.assertEqual(result[side]['next_row_different_from_prepared_words'], 0)
            self.assertEqual(result[side]['shared_qk_differences_between_halves'], 0)
        self.assertEqual(result['prepared_bit_differences'], dict(q=120, k=38, v=22, decay=0, beta=0))
        for name, ulp in (('q', 3), ('k', 2), ('v', 2), ('decay', 0), ('beta', 0)):
            self.assertEqual(result['prepared_drift'][name]['max_ulp'], ulp)
        self.assertEqual(result['raw_drift']['max_ulp'], 33)
        self.assertEqual(result['raw_drift']['worst_ulp_index'], 49)
        self.assertEqual(result['next_row_drift']['max_ulp'], 4)
        self.assertEqual(result['prepared_counterfactual_raw_bits']['all_actual'], 'b69a7f89')
        self.assertEqual(result['prepared_counterfactual_raw_bits']['beta_only'], 'b69a7f68')
        self.assertGreater(result['qk_cancellation_condition_double_observer']['expected'], 1.)

    def test_real_raw_capture_stays_diagnostic_and_cannot_invent_prepared_data(self):
        captured = path.parent/'tests/data/qgt52-head47-raw-context-20261024.log'
        text = captured.read_text()
        context = raw['parse'](text)
        self.assertEqual((context['head'], context['lane'], context['word']), (47, 49, 820975))
        self.assertEqual((context['EXPECTED_RAW'][49], context['ACTUAL_RAW'][49]), (0xb69a7f68, 0xb69a7f89))
        self.assertEqual((context['expected_bits'], context['actual_bits']), (0x3a86371e, 0x3a86373a))
        report = raw['analyze'](context)
        self.assertFalse(report['log_authenticated'])
        self.assertEqual(report['predicted_gated_bits'], '3a86373b')
        with self.assertRaises(ValueError):
            probe['parse'](text) # raw-only record cannot silently mint a prepared/NEXT-row context

    def test_closed_form_and_boundaries(self):
        for head in (0, 47):
            for lane in (0, 49, 63, 64, 127):
                result = probe['analyze'](probe['parse'](fixture(head, lane)))
                self.assertEqual((result['head'], result['lane']), (head, lane))
                self.assertFalse(result['log_authenticated'])
                for side in ('expected', 'actual'):
                    self.assertEqual(result[side]['raw_from_prepared_bits'], result[side]['reported_raw_bits'])
                    self.assertEqual(result[side]['raw_from_next_row_bits'], result[side]['reported_raw_bits'])
                    self.assertEqual(result[side]['next_row_different_from_prepared_words'], 0)
                    self.assertEqual(result[side]['shared_qk_differences_between_halves'], 0)
                self.assertEqual(result['prepared_bit_differences'], dict(q=0, k=0, v=0, decay=0, beta=1))
                self.assertEqual(result['prepared_drift']['beta']['max_relative_nonzero_reference'], .25)
                self.assertEqual(result['prepared_drift']['beta']['max_ulp'], 0x200000)
                self.assertEqual(result['prepared_drift']['beta']['worst_ulp_index'], 0)
                for name in ('q', 'k', 'v', 'decay'):
                    self.assertEqual(result['prepared_drift'][name]['max_ulp'], 0)
                    self.assertIsNone(result['prepared_drift'][name]['worst_ulp_index'])
                for side in ('expected', 'actual'):
                    self.assertEqual(result[side]['next_row_vs_prepared_reconstruction']['max_ulp'], 0)
                    self.assertEqual(result['qk_cancellation_condition_double_observer'][side], 1.)
                for part in ('q_only', 'k_only', 'v_only'):
                    self.assertEqual(result['prepared_counterfactual_raw_bits'][part], result['expected']['reported_raw_bits'])
                for part in ('beta_only', 'all_actual'):
                    self.assertEqual(result['prepared_counterfactual_raw_bits'][part], result['actual']['reported_raw_bits'])

    def test_distinguishes_returned_state_and_preparation(self):
        context = probe['parse'](fixture())
        context['ACTUAL_NEXT_ROW'][0] = raw['bits'](.5)
        context['ACTUAL_PREPARED_1'][7] = raw['bits'](.125)
        result = probe['analyze'](context)
        self.assertNotEqual(result['actual']['raw_from_next_row_bits'], result['actual']['raw_from_prepared_bits'])
        self.assertEqual(result['actual']['next_row_different_from_prepared_words'], 1)
        self.assertEqual(result['actual']['shared_qk_differences_between_halves'], 1)
        self.assertEqual(result['actual']['next_row_vs_prepared_reconstruction']['different_bits'], 1)
        self.assertEqual(result['actual']['next_row_vs_prepared_reconstruction']['worst_ulp_index'], 0)

    def test_drift_metrics_signed_zeros_opposite_signs_and_zero_reference(self):
        bits = raw['bits']
        result = probe['drift']([bits(-0.), bits(-1.), bits(0.), bits(2.)], [bits(0.), bits(-1.0000001192092896), bits(.125), bits(1.)])
        self.assertEqual(result['different_bits'], 4)
        self.assertEqual(result['nonzero_against_zero_reference'], 1)
        self.assertEqual(result['max_ulp'], bits(.125))
        self.assertEqual(result['worst_ulp_index'], 2)
        self.assertEqual(result['max_relative_nonzero_reference'], .5)
        self.assertEqual(result['worst_relative_index'], 3)
        self.assertEqual(probe['drift']([bits(-0.)], [bits(0.)])['max_ulp'], 0)
        self.assertEqual(probe['drift']([bits(-1.)], [bits(1.)])['max_ulp'], 2*bits(1.))
        for left, right in (([], []), ([0], []), ([0x7f800000], [0]), ([0], [0x7fc00000])):
            with self.assertRaises(ValueError):
                probe['drift'](left, right)

    def test_zero_qk_condition_is_not_infinity_or_an_authentication_grant(self):
        context = probe['parse'](fixture())
        for side in ('EXPECTED', 'ACTUAL'):
            for half in (0, 1):
                context[side+'_PREPARED_'+str(half)][:256] = [0]*256
        result = probe['analyze'](context)
        self.assertEqual(result['qk_cancellation_condition_double_observer'], dict(expected=None, actual=None))
        self.assertEqual(result['prepared_counterfactual_raw_bits']['all_actual'], '00000000')
        self.assertFalse(result['log_authenticated'])
        # This intentionally inconsistent CPU fixture is reported,not accepted
        # as a source/model/producer proof by the read-only analyzer.
        self.assertNotEqual(result['expected']['raw_from_prepared_bits'], result['expected']['reported_raw_bits'])

    def test_all_missing_duplicate_truncated_and_malformed_rows(self):
        text = fixture()
        rows = text.splitlines()
        added = [i for i, row in enumerate(rows) if row.startswith('DEBUG_CPU_PREPARED_STATE_CONTEXT') or row.startswith(probe['PREFIXES'])]
        self.assertEqual(len(added), 197)  # marker+4*41prepared+2*16state rows
        for i in added:
            for bad in ('\n'.join(rows[:i]+rows[i+1:]), text+'\n'+rows[i]):
                with self.assertRaises(ValueError):
                    probe['parse'](bad)
        bad = [text.replace('PREPARED_0 word000', 'PREPARED_2 word000', 1),
               text.replace('PREPARED_0 word000', 'PREPARED_0 word001', 1),
               text.replace('PREPARED_0 word000', 'PREPARED_0 word'+'9'*10000, 1),
               text.replace('PREPARED_0 word000 3e800000', 'PREPARED_0 word000 invalid', 1),
               text.replace('PREPARED_0 word320 3f000000 3f000000', 'PREPARED_0 word320 3f000000', 1),
               text.replace('PREPARED_0 word320 3f000000 3f000000', 'PREPARED_0 word320 3f000000 3f000000 00000000', 1),
               text.replace(probe['MARKER'], probe['MARKER']+' corrupted')]
        for item in bad:
            with self.assertRaises(ValueError):
                probe['parse'](item)

    def test_finite_normal_domain_and_gate_bounds(self):
        for name in probe['SIZES']:
            for word in (1, 0x807fffff, 0x47800000, 0x7f800000, 0x7fc00000):
                context = probe['parse'](fixture())
                context[name][127] = word
                with self.assertRaises(ValueError):
                    probe['analyze'](context)
        for side in ('EXPECTED', 'ACTUAL'):
            for lane in (320, 321):
                for value in (-1., 1.125):
                    context = probe['parse'](fixture())
                    context[side+'_PREPARED_0'][lane] = raw['bits'](value)
                    with self.assertRaises(ValueError):
                        probe['analyze'](context)

    def test_read_only_bounded_cli_and_old_log_refusal(self):
        with tempfile.TemporaryDirectory(dir='/private/tmp', prefix='pioneer-qgt-prepared-analysis-') as temporary:
            log = pathlib.Path(temporary)/'context.log'
            old = fixture().split(probe['MARKER'], 1)[0]
            for content, status in ((fixture().encode(), 0), (old.encode(), 2), (b'no context', 2), (b'\xff', 2), (b'x'*(1024*1024+1), 2)):
                log.write_bytes(content)
                result = subprocess.run([sys.executable, str(path), str(log)], capture_output=True, text=True, timeout=5)
                self.assertEqual(result.returncode, status)
                self.assertEqual(log.read_bytes(), content)
                if not status:
                    self.assertFalse(json.loads(result.stdout)['log_authenticated'])
                    self.assertFalse(result.stderr)
                else:
                    self.assertFalse(result.stdout)
                    self.assertTrue(result.stderr.startswith('STOP_CPU_PREPARED_CONTEXT_ANALYSIS:'))


if __name__ == '__main__':
    unittest.main()
