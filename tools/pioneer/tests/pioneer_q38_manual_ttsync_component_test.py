#!/usr/bin/env python3
"""Actual fixed wait bytes plus SYNTHETIC completion/production-role tests.

NOT a full worker arithmetic,asynchronous Tensix,native-owner,original-model,
hardware or performance test. In particular the blocking primitive does not
establish the required independently owned override-clear launch invariant.
"""
import ast
import hashlib
import os
import pathlib
import struct
import subprocess
import sys
from compare_pioneer_q38_observed_trace import require, stable_bytes

WORDS = (0xffe807b7, 0x00478793, 0x0007a023, 0x0007a703,
         0x00077713, 0x0ff0000f, 0x00008067)
ADDRESS = 0xffe80004


def pins(path):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
                if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))


def snapshot(directory, records):
    require(directory.is_absolute() and directory.is_dir() and not directory.is_symlink(), 'absolute image')
    require(bool(records), 'independently frozen pins required')
    result = {}
    for name, size, digest in records:
        data = stable_bytes(directory / name, size)
        require(hashlib.sha256(data).hexdigest() == digest, 'whole frozen image ' + name)
        result[name] = data
    return result


class WaitModel:
    """Independent RV32 seven-word interpreter,NOT a silicon timing model."""
    def __init__(self, delay, returned, bypass=False):
        self.regs = [0] * 32
        self.pc = self.ticks = self.stores = self.loads = 0
        self.remaining, self.returned, self.bypass = delay, returned, bypass
        self.ordered = self.fenced = self.returned_to_caller = False

    def run(self, budget=8192):
        while self.ticks < budget:
            self.ticks += 1
            op = WORDS[self.pc // 4]
            code, rd, f3, rs1, rs2 = op & 127, (op >> 7) & 31, (op >> 12) & 7, (op >> 15) & 31, (op >> 20) & 31
            immediate = op >> 20
            if immediate & 2048:
                immediate -= 4096
            if code == 0x37:
                self.regs[rd] = op & 0xfffff000
            elif code == 0x13:
                if f3 == 0:
                    self.regs[rd] = (self.regs[rs1] + immediate) & 0xffffffff
                else:
                    require(f3 == 7 and rd == rs1 and immediate == 0, 'actual dependent ANDI')
                    self.regs[rd] &= immediate
            elif code == 0x23:
                require(f3 == 2 and self.regs[rs1] == ADDRESS and rs2 == 0, 'exact discarded ordering store')
                self.ordered = True
                self.stores += 1
            elif code == 3:
                require(f3 == 2 and self.regs[rs1] + immediate == ADDRESS and self.ordered, 'exact ordered load')
                if self.remaining != 0 and not self.bypass:
                    # RISC PC/registers do NOT advance while this load is
                    # pending. Independent fabricated backend ticks only.
                    if self.remaining is not None:
                        self.remaining -= 1
                    continue
                self.loads += 1
                self.regs[rd] = self.returned
            elif code == 0x0f:
                require(self.remaining == 0, 'EXPECTED_BYPASS_COUNTEREXAMPLE_NOT_NATIVE_REJECTION')
                require(self.loads == 1 and self.regs[14] == 0, 'undefined bits ignored after dependent use')
                self.fenced = True
            else:
                require(code == 0x67 and self.fenced and self.stores == self.loads == 1, 'return only after actual completion')
                self.returned_to_caller = True
                return True
            self.pc += 4
        return False


def role_checks(here):
    role = here.parent / 'PioneerQ38ManualTTSyncRole.h'
    required = {
        'PIONEER_Q38_MANUAL_TTSYNC_IMAGE': '1', 'PIONEER_Q38_MANUAL_TTSYNC_PARENT_BINDING': '1',
        'PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE': '1', 'PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING': '1',
        'PIONEER_Q38_Q4_CONSTANT_ISSUE': '1', 'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_WORKER_IMAGE': '1',
        'PIONEER_Q38_WORK_TWIN_BATCH_PREFETCH': '1', 'PIONEER_Q38_STATIC_VECTOR': '1',
        'PIONEER_Q38_REPLAY_OPERAND': '1', 'PIONEER_Q38_NATIVE_OPERAND_STORE': '1',
        'PIONEER_Q38_VECTOR_INLINE': '',
    }
    def compile_roles(values, late=False):
        text = ''.join('#define ' + k + ' ' + v + '\n' for k, v in values.items())
        text += '#include "' + str(role) + '"\n'
        if late:
            text = '#include "' + str(role) + '"\n' + text
        return subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Werror', '-x', 'c++', '-fsyntax-only', '-'],
                              input=text, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).returncode
    require(compile_roles({}) == compile_roles(required) == 0, 'default-off and complete role')
    refused = 0
    for name in required:
        value = dict(required)
        del value[name]
        require(compile_roles(value) != 0, 'missing required production role ' + name)
        refused += 1
    for name in tuple(required)[:7]:
        for bad in ('0', '2', ''):
            require(compile_roles({**required, name: bad}) != 0, 'malformed exact production role ' + name)
            refused += 1
    forbidden = ('_KERNEL_MODE', 'PIONEER_DRIVER_BUILD_TAG', '__FAST_MATH__',
                 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
                 'PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT',
                 'PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE', 'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT',
                 'PIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT', 'PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT',
                 'PIONEER_Q38_TOKEN_MATRIX_PHASE_IMAGE', 'PIONEER_Q38_WORK_PEER_PHASE_PROFILE',
                 'PIONEER_Q38_WORK_FRONTEND_BARRIER', 'PIONEER_Q38_WORK_RAW_UNPACK_CACHE',
                 'PIONEER_Q38_WORK_LOCAL_Q4', 'PIONEER_Q38_PREDECODED_Q4', 'PIONEER_Q38_PREPARED_Q4_BLOCK',
                 'PIONEER_Q38_PREPARED_Q4_AFFINE', 'PIONEER_Q38_NATIVE_Q8', 'PIONEER_Q38_BFLOAT_PANEL',
                 'PIONEER_Q38_Q4_GROUP32')
    for name in forbidden:
        for bad in ('0', '1'):
            require(compile_roles({**required, name: bad}) != 0, 'foreign production role ' + name)
            refused += 1
    require(compile_roles(required, late=True) != 0, 'late enable production role')
    return refused + 1


def main():
    require(len(sys.argv) == 3, 'ORIGINAL_CONSTANT_Q4 NEW_MANUAL_TTSYNC')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'no role override')
    here = pathlib.Path(__file__).resolve().parent
    old, new = [pathlib.Path(p) for p in sys.argv[1:]]
    before = snapshot(old, pins(here.parent / 'build_qwen38_q4_constant_issue_worker_image.py'))
    after = snapshot(new, pins(here.parent / 'build_qwen38_manual_ttsync_worker.py'))
    require(tuple(struct.unpack('<7I', after['text.bin'][:28])) == WORDS, 'complete actual wait routine')
    require(after['text.bin'][28:88] == bytes(60), 'unreachable original-slot padding')
    for name in ('boot', 'globalworker', 'precise', 'dense', 'ffn', 'service', 'gdn'):
        require(before[name + '.bin'] == after[name + '.bin'], 'entire unchanged non-matrix payload ' + name)
    successes = 0
    for delay in (0, 1, 7, 31, 4099):
        for undefined in (0, 1, 0xdeadbeef, 0xffffffff):
            m = WaitModel(delay, undefined)
            require(m.run() and m.returned_to_caller and m.fenced, 'delayed synthetic completion')
            successes += 1
    lost = WaitModel(None, 0xdeadbeef)
    require(not lost.run(2048) and lost.pc == 12 and not lost.loads and not lost.returned_to_caller,
            'missing completion keeps the actual load pending,not false ACK or local timeout')
    original = (lost.pc, lost.stores, lost.loads, tuple(lost.regs))
    require(not lost.run(4096) and original == (lost.pc, lost.stores, lost.loads, tuple(lost.regs)), 'pending load remains inert')
    bypass = WaitModel(31, 1, bypass=True)
    try:
        bypass.run()
    except ValueError as error:
        require(str(error) == 'EXPECTED_BYPASS_COUNTEREXAMPLE_NOT_NATIVE_REJECTION', 'exact bypass counterexample')
    else:
        raise ValueError('bypass must not be accepted as completed work')
    refused = role_checks(here)
    print(f'PASS_MANUAL_TTSYNC_COMPONENT actualRoutineWords7 syntheticDelayedCases{successes} missingCompletionPending1 '
          f'expectedBypassCounterexample1 productionRolePositives2 productionRoleRefusals{refused};'
          'NOT full-worker arithmetic,async Tensix,native lifecycle,original-model,hardware,Work or speed qualification')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print('MANUAL_TTSYNC_COMPONENT_REFUSAL ' + str(error), file=sys.stderr)
        sys.exit(1)
