#!/usr/bin/env python3
"""Emit fixed complete QTP1/QTP2 or separately named QTP3 DATA. NEVER install/run."""
import ast
import hashlib
import os
import pathlib
import stat
import sys

def require(condition, message):
    if not condition:
        raise ValueError(message)

def pins(path, key):
    tree = ast.parse(path.read_text())
    found = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == key for t in n.targets)]
    require(len(found) == 1 and found[0], f'fixed {key} pin set required')
    return found[0]

def authenticated(directory, records):
    result = {}
    for name, size, digest in records:
        with (directory/name).open('rb') as source:
            before = os.fstat(source.fileno())
            require(stat.S_ISREG(before.st_mode) and before.st_size == size, f'artifact {name}: type/length')
            data = source.read(size+1)
            after = os.fstat(source.fileno())
            identity = lambda s: (s.st_dev, s.st_ino, s.st_size, s.st_mtime_ns, s.st_ctime_ns)
            require(identity(before) == identity(after) and len(data) == size, f'artifact {name}: changed source')
        require(hashlib.sha256(data).hexdigest() == digest, f'artifact {name}: fixed SHA256')
        result[name] = data  # emit THIS same immutable authenticated snapshot;never reopen
    return result

def require_complete_sections(units, sources):
    require(all(any(unit[0] is source for source in sources) for unit in units),
            'section coverage: unauthenticated source object')
    for source in sources:
        mapped = [unit[1]+'.bin' for unit in units if unit[0] is source]
        expected = {name for name in source if name.endswith('.bin')}
        require(len(mapped) == len(set(mapped)) and set(mapped) == expected,
                'section coverage: every authenticated code section exactly once')

def main():
    require(len(sys.argv) == 5 and sys.argv[4] in ('--cancel-boundary', '--no-cancel', '--precise-activation', '--pushed-completion', '--fault-marker', '--precise-beta', '--job-boot-fold', '--token-frontier', '--retained-conv-closure', '--commit-boundary-seals', '--cohort-local-sources', '--packet-identity-boundary', '--tid-boundary', '--counter-gather', '--empty-receipt-poll', '--flat-matrix-grammar', '--tile-prefetch', '--batched-prefetch', '--twin-batch-prefetch', '--batched-output-closure', '--wide-gdn', '--token-phase-profile', '--constant-q4-issue', '--token-matrix-phase', '--manual-ttsync'), 'explicit exact FP32 token variant required')
    require(not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ), 'role overrides refused')
    root, worker, output = map(pathlib.Path, sys.argv[1:4])
    require(all(p.is_absolute() for p in (root, worker, output)) and not output.exists() and not output.is_symlink(), 'new absolute output required')
    here = pathlib.Path(__file__).resolve().parent
    matrix_phase = sys.argv[4] == '--token-matrix-phase'
    manual_ttsync = sys.argv[4] == '--manual-ttsync'
    constant_q4 = manual_ttsync or matrix_phase or sys.argv[4] == '--constant-q4-issue'
    phase_profile = constant_q4 or sys.argv[4] == '--token-phase-profile'
    wide_gdn = phase_profile or sys.argv[4] == '--wide-gdn'
    output_closure = wide_gdn or sys.argv[4] == '--batched-output-closure'
    twin_batch_prefetch = output_closure or sys.argv[4] == '--twin-batch-prefetch'
    batched_prefetch = twin_batch_prefetch or sys.argv[4] == '--batched-prefetch'
    prefetch = batched_prefetch or sys.argv[4] == '--tile-prefetch'
    flat_grammar = prefetch or sys.argv[4] == '--flat-matrix-grammar'
    empty_receipt = flat_grammar or sys.argv[4] == '--empty-receipt-poll'
    counter_gather = empty_receipt or sys.argv[4] == '--counter-gather'
    tid_boundary = counter_gather or sys.argv[4] == '--tid-boundary'
    packet_identity = tid_boundary or sys.argv[4] == '--packet-identity-boundary'
    local_sources = packet_identity or sys.argv[4] == '--cohort-local-sources'
    commit_boundary = local_sources or sys.argv[4] == '--commit-boundary-seals'
    retained_cohort = commit_boundary or sys.argv[4] == '--retained-conv-closure'
    frontier = retained_cohort or sys.argv[4] == '--token-frontier'
    job_boot_fold = frontier or sys.argv[4] == '--job-boot-fold'
    precise_beta = job_boot_fold or sys.argv[4] == '--precise-beta'
    fault = precise_beta or sys.argv[4] == '--fault-marker'
    pushed = fault or sys.argv[4] == '--pushed-completion'
    precise = pushed or sys.argv[4] == '--precise-activation'
    cancel = precise or sys.argv[4] == '--cancel-boundary'
    r = authenticated(root, pins(here/'build_qwen38_token_phase_profile_root.py', 'PINS') if phase_profile else pins(here/'build_qwen38_batched_output_closure_root.py', 'PINS') if output_closure else pins(here/'build_qwen38_batched_prefetch_root.py', 'PINS') if batched_prefetch else pins(here/'build_qwen38_flat_matrix_grammar_root.py', 'PINS') if flat_grammar else pins(here/'build_qwen38_empty_receipt_poll_root.py', 'PINS') if empty_receipt else pins(here/'build_qwen38_counter_gather_root.py', 'PINS') if counter_gather else pins(here/'build_qwen38_tid_boundary_root.py', 'PINS') if tid_boundary else pins(here/'build_qwen38_packet_identity_boundary_root.py', 'PINS') if packet_identity else pins(here/'build_qwen38_cohort_local_sources_root.py', 'PINS') if local_sources else pins(here/'build_qwen38_commit_boundary_root.py', 'PINS') if commit_boundary else pins(here/'build_qwen38_retained_cohort_root.py', 'PINS') if retained_cohort else pins(here/'build_qwen38_token_fault_marker_image.py', 'FRONTIER_PINS' if frontier else 'JOB_FOLD_PINS' if job_boot_fold else 'PINS') if fault else pins(here/'build_qwen38_token_root.py', 'PINS_CANCEL_F32_PUSHED' if pushed else 'PINS_CANCEL_F32_PRECISE' if precise else 'PINS_CANCEL_F32' if cancel else 'PINS_F32'))
    w = authenticated(worker, pins(here/'build_qwen38_manual_ttsync_worker.py' if manual_ttsync else here/'build_qwen38_token_matrix_phase_worker.py' if matrix_phase else here/'build_qwen38_q4_constant_issue_worker_image.py' if constant_q4 else here/'build_qwen38_wide_gdn_worker_image.py' if wide_gdn else here/'build_qwen38_token_twin_batch_prefetch_worker.py' if twin_batch_prefetch else here/'build_qwen38_token_batched_prefetch_worker.py' if batched_prefetch else here/'build_qwen38_token_prefetch_worker.py' if prefetch else here/'build_qwen38_precise_beta_worker_image.py' if precise_beta else here/'build_qwen38_pushed_worker_image.py' if pushed else here/'tests/run_pioneer_q38_precise_worker_tests.py' if precise else here/'build_qwen38_global_worker_image.py', 'PINS'))
    units = [(r, 'boot', 'RootBoot', 0x6000), (r, 'text', 'RootText', 0x40000),
             (r, 'terminal', 'RootTerminal', 0x60000), (r, 'conv', 'RootAux', 0x64000)]
    units += [(w, name, label, address) for name, label, address in (
        ('boot', 'PeerBoot', 0x6000), ('globalworker', 'PeerFront', 0x20000), ('dense', 'PeerDense', 0x40000),
        ('ffn', 'PeerFfn', 0x60000), ('service', 'PeerService', 0x68000), ('affine', 'PeerAffine', 0x70b00),
        ('text', 'PeerText', 0x72000), ('gdn', 'PeerGdn', 0xb8000))]
    if precise:
        units.insert(6, (w, 'precise', 'PeerPrecise', 0x30000))
    if prefetch:
        units.insert(next(i for i, unit in enumerate(units) if unit[2] == 'PeerAffine'),
                     (w, 'fetch', 'PeerFetch', 0x70000))
    require_complete_sections(units, (r, w))
    lines = ['/* SPDX-License-Identifier: MIT */',
             '// Mechanically generated from COMPLETE fixed FP32 persistent-token/QGW1 artifacts.',
             '// IMMUTABLE DATA ONLY. No native owner,source/init,journal,RUN/STOP or cleanup grant.',
             '#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_PRECISE_WORKER_IMAGE)',
             '#error Fixed QTP1/QGW1 program DATA cannot select the distinct precise ROOT/worker family',
             '#endif']
    if pushed:
        lines = ['/* SPDX-License-Identifier: MIT */',
                 '// Mechanically generated from fixed experimental warm-admission ROOT/QGW3 artifacts.',
                 '// IMMUTABLE DATA ONLY in a DISTINCT namespace, never selected by an old native owner.',
                 '// Full-model/native journal/restore/Work/physical/TPS qualification remains separate.']
        if fault:
            lines[1] = '// Mechanically generated from fixed fault-reporting ROOT and unchanged QGW3 artifacts.'
            lines[2] = '// Separate immutable DATA namespace; the native selector is COMPILE-ONLY.'
        if precise_beta:
            lines[1] = '// Mechanically generated from fixed fault-reporting ROOT and precise-beta QGW3 artifacts.'
            if job_boot_fold:
                lines[1] = '// Mechanically generated from fixed job-boot-fold/eight-way/fault ROOT and precise-beta QGW3 artifacts.'
                selector = 'PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS' if empty_receipt else 'PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS' if counter_gather else 'PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS' if tid_boundary else 'PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS' if packet_identity else 'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS' if local_sources else 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS' if commit_boundary else 'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS' if retained_cohort else 'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS' if frontier else 'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS'
                if flat_grammar:
                    selector = 'PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS'
                if prefetch:
                    selector = 'PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS'
                if batched_prefetch:
                    selector = 'PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS'
                if twin_batch_prefetch:
                    selector = 'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS'
                if output_closure:
                    selector = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS'
                if wide_gdn:
                    selector = 'PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS'
                if phase_profile:
                    selector = 'PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS'
                if constant_q4:
                    selector = 'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS' if matrix_phase else 'PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS'
                if manual_ttsync:
                    selector = 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS'
                lines += ['#include "PioneerQ38TokenManualTTSyncProgramsRole.h"' if manual_ttsync else '#include "PioneerQ38TokenMatrixPhaseProgramsRole.h"' if matrix_phase else '#include "PioneerQ38TokenConstantQ4ProgramsRole.h"' if constant_q4 else '#include "PioneerQ38TokenPhaseProfileProgramsRole.h"' if phase_profile else '#include "PioneerQ38TokenWideGdnProgramsRole.h"' if wide_gdn else '#include "PioneerQ38TokenBatchedOutputClosureProgramsRole.h"' if output_closure else '#include "PioneerQ38TokenTwinBatchPrefetchProgramsRole.h"' if twin_batch_prefetch else '#include "PioneerQ38TokenBatchedPrefetchProgramsRole.h"' if batched_prefetch else '#include "PioneerQ38TokenPrefetchProgramsRole.h"' if prefetch else '#include "PioneerQ38TokenFlatMatrixProgramsRole.h"' if flat_grammar else '#include "PioneerQ38TokenEmptyReceiptPollProgramsRole.h"' if empty_receipt else '#include "PioneerQ38TokenCounterGatherProgramsRole.h"' if counter_gather else '#include "PioneerQ38TokenTidBoundaryProgramsRole.h"' if tid_boundary else '#include "PioneerQ38TokenPacketIdentityBoundaryProgramsRole.h"' if packet_identity else '#include "PioneerQ38TokenCohortLocalSourcesProgramsRole.h"' if local_sources else '#include "PioneerQ38TokenCommitBoundaryProgramsRole.h"' if commit_boundary else '#include "PioneerQ38TokenRetainedCohortProgramsRole.h"' if retained_cohort else '#include "PioneerQ38TokenFrontierProgramsRole.h"' if frontier else '#include "PioneerQ38TokenJobBootFoldProgramsRole.h"',
                          '#if !defined('+selector+') || '+selector+' != 1',
                          '#error Exact separate job boot fold program DATA selection required', '#endif']
            else:
                lines += ['#include "PioneerQ38TokenPreciseBetaProgramsRole.h"']
    elif precise:
        lines[1] = '// Mechanically generated from COMPLETE fixed precise persistent-token/QGW2 artifacts.'
        lines += ['#include "PioneerQ38TokenPreciseProgramsRole.h"']
    else:
        lines += ['#ifdef PIONEER_Q38_TOKEN_PRECISE_PROGRAMS', '#include "PioneerQ38TokenPrecisePrograms.h"', '#else']
    program_namespace = 'PioneerQ38TokenRetainedCohortPrograms' if retained_cohort else 'PioneerQ38TokenFrontierPrograms' if frontier else 'PioneerQ38TokenJobBootFoldPrograms' if job_boot_fold else 'PioneerQ38TokenPreciseBetaPrograms' if precise_beta else 'PioneerQ38TokenFaultPrograms' if fault else 'PioneerQ38TokenPushedPrograms' if pushed else 'PioneerQ38TokenPrograms'
    footprint_namespace = 'PioneerQ38TokenPushedL1' if pushed else 'PioneerQ38TokenL1'
    if commit_boundary:
        program_namespace = 'PioneerQ38TokenCommitBoundaryPrograms'
    if local_sources:
        program_namespace = 'PioneerQ38TokenCohortLocalSourcesPrograms'
    if packet_identity:
        program_namespace = 'PioneerQ38TokenPacketIdentityBoundaryPrograms'
    if tid_boundary:
        program_namespace = 'PioneerQ38TokenTidBoundaryPrograms'
    if counter_gather:
        program_namespace = 'PioneerQ38TokenCounterGatherPrograms'
    if empty_receipt:
        program_namespace = 'PioneerQ38TokenEmptyReceiptPollPrograms'
    if flat_grammar:
        program_namespace = 'PioneerQ38TokenFlatMatrixPrograms'
    if prefetch:
        program_namespace = 'PioneerQ38TokenPrefetchPrograms'
    if batched_prefetch:
        program_namespace = 'PioneerQ38TokenBatchedPrefetchPrograms'
    if twin_batch_prefetch:
        program_namespace = 'PioneerQ38TokenTwinBatchPrefetchPrograms'
    if output_closure:
        program_namespace = 'PioneerQ38TokenBatchedOutputClosurePrograms'
    if wide_gdn:
        program_namespace = 'PioneerQ38TokenWideGdnPrograms'
    if phase_profile:
        program_namespace = 'PioneerQ38TokenPhaseProfilePrograms'
    if manual_ttsync:
        program_namespace = 'PioneerQ38TokenManualTTSyncPrograms'
    elif matrix_phase:
        program_namespace = 'PioneerQ38TokenMatrixPhasePrograms'
    elif constant_q4:
        program_namespace = 'PioneerQ38TokenConstantQ4Programs'
    guard = 'PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS_H' if retained_cohort else 'PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS_H' if frontier else 'PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS_H' if job_boot_fold else 'PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS_H' if precise_beta else 'PIONEER_Q38_TOKEN_FAULT_PROGRAMS_H' if fault else 'PIONEER_Q38_TOKEN_PUSHED_PROGRAMS_H' if pushed else 'PIONEER_Q38_TOKEN_PROGRAMS_H'
    if commit_boundary:
        guard = 'PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS_H'
    if local_sources:
        guard = 'PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS_H'
    if packet_identity:
        guard = 'PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS_H'
    if tid_boundary:
        guard = 'PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS_H'
    if counter_gather:
        guard = 'PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS_H'
    if empty_receipt:
        guard = 'PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS_H'
    if flat_grammar:
        guard = 'PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS_H'
    if prefetch:
        guard = 'PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS_H'
    if batched_prefetch:
        guard = 'PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS_H'
    if twin_batch_prefetch:
        guard = 'PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS_H'
    if output_closure:
        guard = 'PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS_H'
    if wide_gdn:
        guard = 'PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS_H'
    if phase_profile:
        guard = 'PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS_H'
    if manual_ttsync:
        guard = 'PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS_H'
    elif matrix_phase:
        guard = 'PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS_H'
    elif constant_q4:
        guard = 'PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS_H'
    lines += [
             '#ifndef '+guard, '#define '+guard,
             '#include "PioneerQ38TokenPushedFootprint.h"' if pushed else '#include "PioneerQ38TokenFootprint.h"',
             'namespace '+program_namespace+' {',
             f'enum {{Family=0x{0x33505451 if pushed else 0x32505451 if precise else 0x31505451:x},Version=1,CancelBoundary={int(cancel)}}};']
    if fault:
        lines += ['enum {FaultMarker=0x51465431,FaultSavedLinkOffset=32}; // LINK DATA,not a retired PC or STOP receipt']
    if precise_beta:
        lines += ['enum {PreciseBeta=1}; // distinct immutable math revision,not an old owner or Work alias']
    if job_boot_fold:
        lines += ['enum {JobBootFold=1}; // distinct ROOT revision,not an old native or Work alias']
    if frontier:
        lines[1] = '// Mechanically generated from fixed token-frontier/fault ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {TokenFrontier=1,FrontierAddress=0x7570c}; // telemetry DATA only;no native reader or lifecycle grant']
    if retained_cohort:
        lines[1] = '// Mechanically generated from fixed retained-Conv-closure/frontier ROOT and unchanged precise-beta QGW3 artifacts.'
        lines[2] = '// Separate immutable DATA namespace; no native owner or shared selector exists.'
        lines += ['enum {RetainedConvClosure=1}; // distinct DATA ONLY;not a native owner or Work alias']
    if commit_boundary:
        lines[1] = '// Mechanically generated from fixed commit-boundary-seal/retained-Conv ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {CommitBoundarySeals=1}; // distinct DATA ONLY;not a native owner or Work alias']
    if local_sources:
        lines[1] = '// Mechanically generated from fixed cohort-local-source/cold-audit/commit-boundary ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {CohortLocalSources=1,ColdSourceFromStatus=1}; // distinct DATA ONLY;not native or Work authority']
    if packet_identity:
        lines[1] = '// Mechanically generated from fixed packet-identity-boundary/cohort-source ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {PacketIdentityBoundary=1}; // distinct DATA ONLY;not native or Work authority']
    if tid_boundary:
        lines[1] = '// Mechanically generated from fixed FIFO-aware/TID-boundary ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {TidBoundary=1,FifoEnabledRequired=1,ActiveTid=0}; // distinct DATA ONLY;no native owner or Work authority']
    if counter_gather:
        lines[1] = '// Mechanically generated from fixed eight-counter FIFO-gather ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {CounterGather=1,CounterGatherReads=8}; // immutable DATA only;not a native or Work grant']
    if empty_receipt:
        lines[1] = '// Mechanically generated from fixed read-source-boundary/empty-receipt-poll ROOT and unchanged precise-beta QGW3 artifacts.'
        lines += ['enum {ReadSourceBoundary=1,EmptyReceiptPoll=1}; // DATA only;not native,device or Work authority']
    if flat_grammar:
        lines[1] = '// Mechanically generated from fixed ALL-TU flat QGW3 grammar ROOT and unchanged precise-beta worker artifacts.'
        lines += ['enum {FlatMatrixGrammar=1}; // DATA only;no native,device,model quality or Work authority']
    if prefetch:
        lines[1] = '// Mechanically generated from fixed flat QGW3 ROOT and current full-model prefetch worker artifacts.'
        lines += ['enum {TilePrefetch=1,RawBuffers=2,FetchAddress=0x70000}; // DATA ONLY;no native or Work grant']
    if batched_prefetch:
        lines[1] = '// Mechanically generated from fixed matching batched-counter ROOT and batched-prefetch QGW3 worker artifacts.'
        lines += ['enum {BatchedPrefetch=1,Q8Batch=4,Q4Batch=2,Q6Batch=2}; // distinct DATA ONLY;old ROOT/native counters incompatible']
    if twin_batch_prefetch:
        lines[1] = '// Mechanically generated from fixed matching batched-counter ROOT and copy-free twin-batch QGW3 worker artifacts.'
        lines += ['enum {TwinBatchPrefetch=1,FullBatchBuffers=2,RawCopyBytes=0,SecondBatchAddress=0x9a000}; // pure DATA ONLY;NO native or Work binding']
    if output_closure:
        lines[1] = '// Mechanically generated from fixed corrected FINAL-output ROOT and unchanged copy-free twin-batch worker artifacts.'
        lines += ['enum {BatchedOutputClosure=1,GlobalOutputReadsPerGroup=10}; // distinct DATA;no old native or Work authority']
    if wide_gdn:
        lines[1] = '// Mechanically generated from fixed corrected FINAL-output ROOT and complete returning wide-GDN twin-batch worker artifacts.'
        lines += ['enum {WideGdn=1,WideGdnDeltaAddress=0x8e600,WideGdnDeltaWords=64,WideGdnImports=13}; // distinct DATA;unchanged saved extent is NOT execution or Work authority']
    if phase_profile:
        lines[1] = '// Mechanically generated from full fixed phase-timing ROOT and unchanged returning wide-GDN worker.'
        lines += ['enum {TokenPhaseProfile=1,PhaseProfileAddress=0x75710,PhaseProfileWords=60}; // diagnostic DATA ONLY;no readout/owner/Work grant']
    if constant_q4:
        lines[1] = '// Mechanically generated from full fixed phase-timing ROOT and complete constant-Q4-issue returning worker.'
        lines += ['enum {ConstantQ4Issue=1,ConstantQ4WorkerRevision=1}; // distinct DATA ONLY;not old native,quality or Work authority']
    if matrix_phase:
        lines[1] = '// Mechanically generated from full fixed timing ROOT and complete returning Q4-job sampler worker.'
        lines += ['enum {MatrixPhase=1,MatrixPhaseAddress=0x84500,MatrixPhaseWords=16,MatrixPhaseVersion=2}; // latest Q4 job DATA,not whole-model timing or completion']
    if manual_ttsync:
        lines[1] = '// Mechanically generated from full fixed timing ROOT and complete manual-completion returning worker.'
        lines += ['enum {ManualTTSync=1,ManualTTSyncRevision=1,ManualTTSyncOverrideClearRequired=1,ManualTTSyncLocalTimeout=0}; // DATA only;native watchdog/owner/quality remain separate']
    for data_set, name, label, address in units:
        data = data_set[name+'.bin']
        require(data and not len(data)%4 and not address%4 and address+len(data) <= 0x180000, f'{label}: extent')
        lines += ['// SHA256 '+hashlib.sha256(data).hexdigest(), f'static const uint32_t {label}[]={{']
        words = [int.from_bytes(data[i:i+4], 'little') for i in range(0, len(data), 4)]
        lines += [' '+','.join(f'0x{word:08x}U' for word in words[i:i+8])+',' for i in range(0, len(words), 8)]
        lines += ['};', f'static_assert(sizeof({label})=={len(data)},"exact complete {label} DATA");']
    lines += ['using '+footprint_namespace+'::Span;',
              f'static inline unsigned CodeSpans(unsigned worker){{return worker>=120?0:worker?{10 if prefetch else 9 if precise else 8}:4;}}',
              'static inline Span CodeSpan(unsigned worker,unsigned span){',
              ' if(worker>=120 || span>=CodeSpans(worker))return {UINT32_MAX,0};', ' if(!worker)switch(span){']
    for i, (_, _, label, address) in enumerate(units[:4]):
        lines += [f'  case {i}:return {{0x{address:x},unsigned(sizeof({label})/4)}};']
    lines += [' }else switch(span){']
    for i, (_, _, label, address) in enumerate(units[4:]):
        lines += [f'  case {i}:return {{0x{address:x},unsigned(sizeof({label})/4)}};']
    lines += [' }return {UINT32_MAX,0};', '}',
              'static inline unsigned CodeWords(unsigned worker){',
              ' unsigned words=0;for(unsigned i=0;i<CodeSpans(worker);i++)words+=CodeSpan(worker,i).words;return words;', '}',
              'static inline uint32_t CodeAddress(unsigned worker,unsigned ordinal){',
              ' if(worker>=120)return UINT32_MAX;',
              ' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
              '  if(ordinal<s.words)return s.address+ordinal*4;', '  ordinal-=s.words;', ' }return UINT32_MAX;', '}',
              'static inline bool ProgramWord(unsigned worker,uint32_t address,uint32_t& output){',
              ' if(worker>=120 || (address&3) || (reinterpret_cast<uintptr_t>(&output)&3))return false;',
              ' const uintptr_t target=reinterpret_cast<uintptr_t>(&output);']
    for _, _, label, _ in units:
        lines += [f' if(target>=reinterpret_cast<uintptr_t>({label}) && target-reinterpret_cast<uintptr_t>({label})<sizeof({label}))return false;']
    lines += [' for(unsigned i=0;i<CodeSpans(worker);i++){const auto s=CodeSpan(worker,i);',
              '  if(address<s.address || address-s.address>=s.words*4)continue;',
              '  const uint32_t* data=nullptr;', '  if(!worker)switch(i){']
    for i, (_, _, label, _) in enumerate(units[:4]):
        lines += [f'   case {i}:data={label};break;']
    lines += ['  }else switch(i){']
    for i, (_, _, label, _) in enumerate(units[4:]):
        lines += [f'   case {i}:data={label};break;']
    lines += ['  }', '  if(!data)return false;', '  output=data[(address-s.address)/4];return true;',
              ' }return false; // no implicit zero-padded holes or runtime-data permission', '}', '}', '#endif']
    if pushed:
        lines += ['static_assert('+program_namespace+'::Family==0x33505451,"distinct proposed QTP3 program DATA");', '']
    elif precise:
        lines += ['static_assert(PioneerQ38TokenPrograms::Family==0x32505451,"late QTP2 program DATA selection");', '']
    else:
        lines += ['#endif', '#ifdef PIONEER_Q38_TOKEN_PRECISE_PROGRAMS',
                  'static_assert(PioneerQ38TokenPrograms::Family==0x32505451,"late QTP2 program DATA selection");',
                  '#else', 'static_assert(PioneerQ38TokenPrograms::Family==0x31505451,"late QTP1 program DATA selection");', '#endif', '']
    # All artifacts,including BOTH complete ELFs,were authenticated BEFORE
    # creating any output. Never replace or repair an existing header.
    encoded = '\n'.join(lines).encode()
    with output.open('xb') as target:
        require(target.write(encoded) == len(encoded), 'complete generated output write')
    print(hashlib.sha256(encoded).hexdigest()+f' token-programs FP32/cancel{int(cancel)}/ALL{len(units)} sections+2fullELFs authenticated/DATA ONLY/NO install or native permission')

if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError) as error:
        print(f'PROGRAM_DATA_REFUSAL {error}', file=sys.stderr)
        sys.exit(2)
