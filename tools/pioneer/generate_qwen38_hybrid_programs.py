#!/usr/bin/env python3
"""Generate pinned QGW5/QGW7/QGW8 program DATA; no execution authority."""
import hashlib
import os
from pathlib import Path
import sys
from generate_qwen38_token_programs import authenticated, require_complete_sections
args=sys.argv[1:];qgwd='--qgwd-dense-q8-inspection' in args
if qgwd:
    if args.count('--qgwd-dense-q8-inspection')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--qgwc-','--qgwd-','--matrix-')) and x!='--qgwd-dense-q8-inspection' for x in args):
        raise SystemExit('ambiguous QGWD selector')
    args.remove('--qgwd-dense-q8-inspection');args.insert(0,'--qgw8-head-inspection')
qgwcnative='--qgwc-srca-native' in args
if qgwcnative:
    if args.count('--qgwc-srca-native')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--qgwc-','--matrix-')) and x!='--qgwc-srca-native' for x in args):
        raise SystemExit('ambiguous QGWC SrcA native selector')
    args.remove('--qgwc-srca-native');args.insert(0,'--qgwc-srca-inspection')
qgwc='--qgwc-srca-inspection' in args
if qgwc:
    if args.count('--qgwc-srca-inspection')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--qgwc-','--matrix-')) and x!='--qgwc-srca-inspection' for x in args):
        raise SystemExit('ambiguous QGWC SrcA selector')
    args.remove('--qgwc-srca-inspection');args.insert(0,'--qgw8-head-inspection')
qgwa='--qgwa-batch-inspection' in args
if qgwa:
    if args.count('--qgwa-batch-inspection')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--matrix-')) and x!='--qgwa-batch-inspection' for x in args):
        raise SystemExit('ambiguous QGWA batch selector')
    args.remove('--qgwa-batch-inspection');args.insert(0,'--qgw8-head-inspection')
qgw8valuenative='--qgw8-dense-value-native' in args
if qgw8valuenative:
    if args.count('--qgw8-dense-value-native')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--matrix-')) and x!='--qgw8-dense-value-native' for x in args):
        raise SystemExit('ambiguous QGW8 dense-value native selector')
    args.remove('--qgw8-dense-value-native');args.insert(0,'--qgw8-dense-value-inspection')
qgw8value='--qgw8-dense-value-inspection' in args
if qgw8value:
    if args.count('--qgw8-dense-value-inspection')!=1 or any(x.startswith(('--qgw8-','--qgwa-','--matrix-')) and x!='--qgw8-dense-value-inspection' for x in args):
        raise SystemExit('ambiguous QGW8 dense-value selector')
    args.remove('--qgw8-dense-value-inspection');args.insert(0,'--qgw8-layer-trace-native' if qgw8valuenative else '--qgw8-layer-trace-inspection')
qgw8inputnative='--qgw8-dense-input-native' in args
if qgw8inputnative:
    if args.count('--qgw8-dense-input-native')!=1 or any(x in args for x in ('--qgw8-dense-input-inspection','--qgw8-dense-stage-native','--qgw8-dense-stage-inspection','--qgw8-layer-trace-native','--qgw8-layer-trace-inspection')):
        raise SystemExit('ambiguous QGW8 dense-input native selector')
    args.remove('--qgw8-dense-input-native');args.insert(0,'--qgw8-dense-input-inspection')
qgw8input='--qgw8-dense-input-inspection' in args
if qgw8input:
    if args.count('--qgw8-dense-input-inspection')!=1 or any(x in args for x in ('--qgw8-dense-stage-native','--qgw8-dense-stage-inspection','--qgw8-layer-trace-native','--qgw8-layer-trace-inspection')):
        raise SystemExit('ambiguous QGW8 dense-input selector')
    args.remove('--qgw8-dense-input-inspection');args.insert(0,'--qgw8-layer-trace-native' if qgw8inputnative else '--qgw8-layer-trace-inspection')
qgw8stagenative='--qgw8-dense-stage-native' in args
if qgw8stagenative:
    if args.count('--qgw8-dense-stage-native')!=1 or any(x in args for x in ('--qgw8-dense-stage-inspection','--qgw8-layer-trace-native','--qgw8-layer-trace-inspection')):
        raise SystemExit('ambiguous QGW8 dense-stage native selector')
    args.remove('--qgw8-dense-stage-native');args.insert(0,'--qgw8-dense-stage-inspection')
qgw8stage='--qgw8-dense-stage-inspection' in args
if qgw8stage:
    if args.count('--qgw8-dense-stage-inspection')!=1 or any(x in args for x in ('--qgw8-layer-trace-native','--qgw8-layer-trace-inspection')):
        raise SystemExit('ambiguous QGW8 dense-stage selector')
    args.remove('--qgw8-dense-stage-inspection');args.insert(0,'--qgw8-layer-trace-native' if qgw8stagenative else '--qgw8-layer-trace-inspection')
qgw8tracenative='--qgw8-layer-trace-native' in args
if qgw8tracenative:
    if args.count('--qgw8-layer-trace-native')!=1 or '--qgw8-layer-trace-inspection' in args:
        raise SystemExit('ambiguous QGW8 layer trace native selector')
    args.remove('--qgw8-layer-trace-native');args.insert(0,'--qgw8-layer-trace-inspection')
qgw8trace='--qgw8-layer-trace-inspection' in args
if qgw8trace:
    if args.count('--qgw8-layer-trace-inspection')!=1 or any(x in args for x in ('--qgw8-coarse-profile-inspection','--qgw8-native-candidate','--qgw8-head-inspection','--matrix-native-candidate','--matrix-attention-inspection')):
        raise SystemExit('ambiguous QGW8 layer trace selector')
    args.remove('--qgw8-layer-trace-inspection');args.insert(0,'--qgw8-head-inspection')
qgw8coarse='--qgw8-coarse-profile-inspection' in args
if qgw8coarse:
    if args.count('--qgw8-coarse-profile-inspection')!=1 or any(x in args for x in ('--qgw8-native-candidate','--qgw8-head-inspection','--matrix-native-candidate','--matrix-attention-inspection')):
        raise SystemExit('ambiguous QGW8 coarse profile selector')
    args.remove('--qgw8-coarse-profile-inspection');args.insert(0,'--qgw8-head-inspection')
qgw8candidate='--qgw8-native-candidate' in args
if qgw8candidate:
    if args.count('--qgw8-native-candidate')!=1 or '--qgw8-head-inspection' in args:raise SystemExit('ambiguous QGW8 candidate selector')
    args.remove('--qgw8-native-candidate');args.insert(0,'--qgw8-head-inspection')
qgw8='--qgw8-head-inspection' in args
if qgw8:
    if args.count('--qgw8-head-inspection')!=1 or '--matrix-native-candidate' in args or '--matrix-attention-inspection' in args:
        raise SystemExit('ambiguous QGW8 selector')
    args.remove('--qgw8-head-inspection')
candidate='--matrix-native-candidate' in args
if candidate:
    if args.count('--matrix-native-candidate')!=1 or '--matrix-attention-inspection' in args:raise SystemExit('ambiguous matrix selector')
    args.remove('--matrix-native-candidate')
matrix=qgw8 or candidate or '--matrix-attention-inspection' in args
if matrix:
    if not candidate and not qgw8:
        if args.count('--matrix-attention-inspection')!=1:raise SystemExit('duplicate matrix selector')
        args.remove('--matrix-attention-inspection')
    if qgwd:from PioneerQ38QgwdDenseProgramPins import ROOT_PINS, WORKER_PINS
    elif qgwc:from PioneerQ38QgwcSrcaProgramPins import ROOT_PINS, WORKER_PINS
    elif qgwa:from PioneerQ38QgwaBatchProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8value:from PioneerQ38Qgw8DenseValueProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8input:from PioneerQ38Qgw8DenseInputProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8stage:from PioneerQ38Qgw8DenseStageProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8trace:from PioneerQ38Qgw8LayerTraceProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8coarse:from PioneerQ38Qgw8CoarseProgramPins import ROOT_PINS, WORKER_PINS
    elif qgw8:from PioneerQ38Qgw8ProgramPins import ROOT_PINS, WORKER_PINS
    else:from PioneerQ38HybridMatrixProgramPins import ROOT_PINS, WORKER_PINS
else:
    from PioneerQ38HybridProgramPins import ROOT_PINS, WORKER_PINS
if not __debug__ or len(args)!=3 or any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ):
    raise SystemExit('usage: generate_qwen38_hybrid_programs.py [--qgw8-layer-trace-native|--qgw8-layer-trace-inspection|--qgw8-coarse-profile-inspection|--qgw8-head-inspection|--qgw8-native-candidate|--matrix-attention-inspection|--matrix-native-candidate] ABS_ROOT ABS_WORKER NEW_ABS_OUTPUT')
root, worker, output=map(Path,args)
if not all(p.is_absolute() for p in (root,worker,output)) or output.exists() or output.is_symlink():
    raise SystemExit('new absolute output and absolute input paths required')
r=authenticated(root,ROOT_PINS);w=authenticated(worker,WORKER_PINS)
units=[(r,'boot','RootBoot',0x6000),(r,'bf16','RootBf16',0x20000),(r,'text','RootText',0x40000),
       (r,'terminal','RootTerminal',0x60000),(r,'conv','RootAux',0x64000)]
units += [(w,name,label,address) for name,label,address in (
 ('boot','PeerBoot',0x6000),('globalworker','PeerFront',0x20000),('bf16producer','PeerBf16',0x30000),
 ('dense','PeerDense',0x40000),('cachedfpu','PeerFpu',0x50000),('ffn','PeerFfn',0x60000),
 ('service','PeerService',0x68000),('affine','PeerAffine',0x70b00),('text','PeerText',0x72000),
 ('gdn','PeerGdn',0x170000))]
require_complete_sections(units,(r,w))
family='0x47505451' if qgwd else '0x46505451' if qgwc else '0x45505451' if qgw8value else '0x44505451' if qgwa else '0x43505451' if qgw8input else '0x42505451' if qgw8stage else '0x41505451' if qgw8trace else '0x39505451' if qgw8coarse else '0x38505451' if qgw8 else '0x37505451' if matrix else '0x35505451'
name='QTPG/QGWD-DENSE-Q8' if qgwd else 'QTPF/QGWC-SRCA' if qgwc else ('QTPE/QGW8-DENSE-VALUE-NATIVE-BUILD-ONLY' if qgw8valuenative else 'QTPE/QGW8-DENSE-VALUE') if qgw8value else 'QTPD/QGWA-BATCH' if qgwa else ('QTPC/QGW8-DENSE-INPUT-NATIVE-BUILD-ONLY' if qgw8inputnative else 'QTPC/QGW8-DENSE-INPUT') if qgw8input else ('QTPB/QGW8-DENSE-STAGE-NATIVE-BUILD-ONLY' if qgw8stagenative else 'QTPB/QGW8-DENSE-STAGE') if qgw8stage else ('QTPA/QGW8-TRACE-NATIVE-BUILD-ONLY' if qgw8tracenative else 'QTPA/QGW8-TRACE') if qgw8trace else 'QTP9/QGW8-COARSE' if qgw8coarse else 'QTP8/QGW8' if qgw8 else 'QTP7/QGW7' if matrix else 'QTP5/QGW5'
lines=['/* SPDX-License-Identifier: MIT */','// Generated '+name+(' candidate code DATA, NOT QUALIFIED; not a runtime capability.' if candidate else ' code DATA, OFFLINE ONLY; not a runtime capability.'),
 '#include "PioneerQ38DenseQ8TokenFootprint.h"' if qgwd else '#include "PioneerQ38HybridTokenFootprint.h"',
 '#if !defined(PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC) || PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC != 1 \\',
 ' || defined(PIONEER_Q38_TOKEN_BF16_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS_H) \\',
 ' || defined(PIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS_H) \\',
 ' || defined(PIONEER_Q38_TOKEN_PROGRAMS_H) || defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H)',
 '#error Exact offline QGW5 program DATA selector required', '#endif']
if matrix:
    lines += ['#if !defined(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION) || PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION != 1 \\',
              ' || !defined(PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT) || PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT != 1 \\']
    if qgwd:
        lines += [' || !defined(PIONEER_Q38_QGWD_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWD_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGWD_PLAN_SIMULATION) || PIONEER_Q38_QGWD_PLAN_SIMULATION != 1 \\',
                  ' || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION != 1 \\',
                  ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
                  ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
                  '#error QGWD program data is isolated inspection only', '#endif']
    elif qgw8tracenative:
        lines += [' || !defined(PIONEER_Q38_QGW8_TRACE_NATIVE_131) || PIONEER_Q38_QGW8_TRACE_NATIVE_131 != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)',
                  '#error QGW8 trace native program requires the isolated tag131 build-only role', '#endif',
                  '#include "PioneerQ38Qgw8TraceNativeProgramRole.h"']
    elif qgw8trace:
        lines += [' || !defined(PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \\',
                  ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \\',
                  ' || defined(TT_PIONEER_DRIVER_RELOAD) || defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_CLIENT) || defined(PIONEER_Q38_FULL_OFFLOAD_CLIENT) \\',
                  ' || defined(PIONEER_Q38_BF16_FULL_NATIVE) || defined(PIONEER_Q38_BF16_FULL_CLIENT) \\',
                  ' || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) \\',
                  ' || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) \\',
                  ' || defined(PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) \\',
                  ' || defined(PIONEER_Q38_QGW8_LAYER_TRACE_IMAGE) || defined(__FAST_MATH__) \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
                  ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
                  '#error QGW8 layer trace data is offline inspection only', '#endif']
    elif qgw8coarse:
        lines += [' || !defined(PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_COARSE_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \\',
                  ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) \\',
                  ' || defined(PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128) || defined(PIONEER_Q38_QGW8_HIDDEN_NATIVE_129) \\',
                  ' || defined(PIONEER_Q38_QGW8_HIDDEN_CLIENT_129) || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY) \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
                  ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
                  '#error QGW8 coarse timing data is offline inspection only', '#endif']
    elif qgwc:
        if qgwcnative:
            lines += [' || !defined(PIONEER_Q38_QGWC_NATIVE_136) || PIONEER_Q38_QGWC_NATIVE_136 != 1 \\',
                      ' || !defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWC_PROGRAMS_INSPECTION != 1 \\',
                      ' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                      ' || !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \\',
                      ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
                      ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
                      '#error QGWC native data requires the isolated tag136 build role', '#endif',
                      '#include "PioneerQ38QgwcNativeRole.h"']
        else:
            lines += [' || !defined(PIONEER_Q38_QGWC_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWC_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGWC_PLAN_SIMULATION) || PIONEER_Q38_QGWC_PLAN_SIMULATION != 1 \\',
                  ' || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \\',
                  ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
                  ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
                  '#error QGWC data requires its isolated offline inspection role', '#endif']
    elif qgw8 and qgw8candidate:
        lines += [' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128) || PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128 != 1 \\',
                  ' || !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)',
                  '#error QGW8 candidate data requires the exclusive tag128 native role', '#endif']
    elif qgw8:
        lines += [' || !defined(PIONEER_Q38_QGW8_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_PROGRAMS_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_QGW8_LAYOUT_INSPECTION) || PIONEER_Q38_QGW8_LAYOUT_INSPECTION != 1 \\',
                  ' || !defined(PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION != 1 \\',
                  ' || (defined(_KERNEL_MODE) && !defined(PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)) \\',
                  ' || ((defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY)) && !defined(PIONEER_Q38_QGW8_NATIVE_INSPECTION)) \\',
                  ' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)',
                  '#error QGW8 data requires the distinct non-loadable inspection role', '#endif']
    elif candidate:
        lines += [' || !defined(PIONEER_Q38_HYBRID_MATRIX_NATIVE_127) || PIONEER_Q38_HYBRID_MATRIX_NATIVE_127 != 1 \\',
                  ' || !defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || PIONEER_Q38_HYBRID_FULL_NATIVE != 1 \\',
                  ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)',
                  '#error QGW7 candidate data requires the exclusive tag127 native role', '#endif']
    else:
        lines += [' || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)',
                  '#error QGW7 data requires the non-loadable matrix-attention inspection role', '#endif']
if qgw8valuenative:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134) || PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134 != 1 \\',
              ' || defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) \\',
              ' || defined(PIONEER_Q38_QGW8_DENSE_VALUE_PROGRAMS_INSPECTION)',
              '#error QGW8 dense-value native program requires its exclusive build-only role', '#endif']
elif qgw8value:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_VALUE_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_DENSE_VALUE_PROGRAMS_INSPECTION != 1 \\',
              ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \\',
              ' || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \\',
              ' || defined(PIONEER_Q38_QGW8_DENSE_INPUT_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION) \\',
              ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
              ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
              '#error QGW8 dense-value program requires exclusive offline DATA inspection', '#endif']
elif qgwa:
    lines += ['#if !defined(PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION) || PIONEER_Q38_QGWA_BATCH_PROGRAMS_INSPECTION != 1 \\',
              ' || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_DRIVER_RELOAD) \\',
              ' || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_HYBRID_FULL_NATIVE) \\',
              ' || defined(PIONEER_Q38_QGWA_BATCH_ROOT_IMAGE) || defined(PIONEER_Q38_QGW8_BFP8_BATCH_WORKER) \\',
              ' || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \\',
              ' || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)',
              '#error QGWA batch program requires exclusive offline DATA inspection', '#endif']
elif qgw8inputnative:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133) || PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133 != 1 \\',
              ' || defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) || defined(PIONEER_Q38_QGW8_DENSE_INPUT_PROGRAMS_INSPECTION)',
              '#error QGW8 dense-input native program requires its exclusive build-only role', '#endif']
elif qgw8input:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_INPUT_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_DENSE_INPUT_PROGRAMS_INSPECTION != 1 \\',
              ' || defined(PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION) || defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132)',
              '#error QGW8 dense-input program requires its exclusive offline role', '#endif']
elif qgw8stagenative:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132) || PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132 != 1',
              '#error QGW8 dense-stage program requires its distinct native role', '#endif']
elif qgw8stage:
    lines += ['#if !defined(PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION) || PIONEER_Q38_QGW8_DENSE_STAGE_PROGRAMS_INSPECTION != 1',
              '#error QGW8 dense-stage program requires its distinct offline role', '#endif']
lines += [
 '#ifndef PIONEER_Q38_HYBRID_PROGRAMS_H','#define PIONEER_Q38_HYBRID_PROGRAMS_H',
 'namespace PioneerQ38HybridPrograms {',
 'enum {Family='+family+',Version=1,CancelBoundary=1,Bf16=1,EogCount=5};']
for source,name,label,address in units:
    data=source[name+'.bin']
    if not data or len(data)%4:
        raise ValueError('unaligned or empty section '+name)
    words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
    lines += ['// SHA256 '+hashlib.sha256(data).hexdigest(),'static const uint32_t '+label+'[]={']
    lines += [' '+','.join('0x%08xU'%v for v in words[i:i+8])+',' for i in range(0,len(words),8)]
    lines += ['};','static_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");']
lines += ['using PioneerQ38HybridTokenL1::Span;',
 'static inline unsigned CodeSpans(unsigned worker){return worker>=120?0:worker?10:5;}',
 'static inline Span CodeSpan(unsigned worker,unsigned span){',
 ' if(worker>=120 || span>=CodeSpans(worker))return {UINT32_MAX,0};',' if(!worker)switch(span){']
for i,(_,_,label,address) in enumerate(units[:5]):
    lines.append('  case %u:return {0x%x,unsigned(sizeof(%s)/4)};'%(i,address,label))
lines += [' }else switch(span){']
for i,(_,_,label,address) in enumerate(units[5:]):
    lines.append('  case %u:return {0x%x,unsigned(sizeof(%s)/4)};'%(i,address,label))
lines += [' }return {UINT32_MAX,0};','}',
 'static inline unsigned CodeWords(unsigned worker){unsigned n=0;for(unsigned i=0;i<CodeSpans(worker);i++)n+=CodeSpan(worker,i).words;return n;}',
 'static inline uint32_t CodeAddress(unsigned worker,unsigned ordinal){',
 ' if(worker>=120)return UINT32_MAX;',
 ' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);if(ordinal<s.words)return s.address+ordinal*4;ordinal-=s.words;}',
 ' return UINT32_MAX;','}',
 'static inline bool ProgramWord(unsigned worker,uint32_t address,uint32_t& output){',
 ' if(worker>=120 || (address&3) || (reinterpret_cast<uintptr_t>(&output)&3))return false;',
 ' const uintptr_t target=reinterpret_cast<uintptr_t>(&output);']
for _,_,label,_ in units:
    lines.append(' if(target>=reinterpret_cast<uintptr_t>(%s) && target-reinterpret_cast<uintptr_t>(%s)<sizeof(%s))return false;'%(label,label,label))
lines += [' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
 '  if(address<s.address || address-s.address>=s.words*4)continue;',
 '  const uint32_t* data=nullptr;', '  if(!worker)switch(i){']
for i,(_,_,label,_) in enumerate(units[:5]):
    lines.append('   case %u:data=%s;break;'%(i,label))
lines += ['  }else switch(i){']
for i,(_,_,label,_) in enumerate(units[5:]):
    lines.append('   case %u:data=%s;break;'%(i,label))
lines += ['  }if(!data)return false;output=data[(address-s.address)/4];return true;',
 ' }return false; // no implicit executable padding or runtime-data access','}',
 '}','#endif','static_assert(PioneerQ38HybridPrograms::Family=='+family+',"late '+('QGW8' if qgw8 else 'QGW7' if matrix else 'QGW5')+' DATA selection");']
encoded=('\n'.join(lines)+'\n').encode()
with output.open('xb') as target:
    if target.write(encoded)!=len(encoded):
        raise OSError('incomplete generated output')
print('PASS PINNED '+('QGWD-DENSE-Q8' if qgwd else 'QGWC-SRCA' if qgwc else 'QGW8-DENSE-VALUE' if qgw8value else 'QGWA-BATCH' if qgwa else 'QGW8-DENSE-INPUT' if qgw8input else 'QGW8-DENSE-STAGE' if qgw8stage else 'QGW8-TRACE' if qgw8trace else 'QGW8-COARSE' if qgw8coarse else 'QGW8' if qgw8 else 'QGW7' if matrix else 'QGW5')+' DATA 15 sections/two authenticated full ELFs; SHA256 '+hashlib.sha256(encoded).hexdigest()+'; NO DEVICE OR NATIVE AUTHORITY')
