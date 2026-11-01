#!/usr/bin/env python3
"""Full-model native initializers with actual frozen zero RV32; NO device IO."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

root_profile=None
twin_batch_prefetch='--twin-batch-prefetch' in sys.argv
if twin_batch_prefetch:
    assert sys.argv.count('--twin-batch-prefetch')==1 and '--batched-prefetch' not in sys.argv
    sys.argv.remove('--twin-batch-prefetch')
batched_prefetch='--batched-prefetch' in sys.argv
if batched_prefetch:
    assert sys.argv.count('--batched-prefetch')==1
    sys.argv.remove('--batched-prefetch')
flat_matrix=None
if '--flat-matrix-root' in sys.argv:
    assert sys.argv.count('--flat-matrix-root')==1
    index=sys.argv.index('--flat-matrix-root')
    assert index+1<len(sys.argv)
    flat_matrix=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert flat_matrix.is_absolute() and flat_matrix.is_dir() and not flat_matrix.is_symlink()
prefetch_worker=None
if '--prefetch-worker' in sys.argv:
    assert sys.argv.count('--prefetch-worker')==1 and flat_matrix is not None
    index=sys.argv.index('--prefetch-worker');assert index+1<len(sys.argv)
    prefetch_worker=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert prefetch_worker.is_absolute() and prefetch_worker.is_dir() and not prefetch_worker.is_symlink()
empty_receipt=None
if '--empty-receipt-root' in sys.argv:
    assert flat_matrix is None,'Distinct ROOT variants cannot alias'
    assert sys.argv.count('--empty-receipt-root')==1
    index=sys.argv.index('--empty-receipt-root')
    assert index+1<len(sys.argv)
    empty_receipt=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert empty_receipt.is_absolute() and empty_receipt.is_dir() and not empty_receipt.is_symlink()
counter_gather=None
if '--counter-gather-root' in sys.argv:
    assert empty_receipt is None and flat_matrix is None, 'Distinct ROOT variants cannot alias'
    assert sys.argv.count('--counter-gather-root')==1
    index=sys.argv.index('--counter-gather-root')
    assert index+1<len(sys.argv)
    counter_gather=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert counter_gather.is_absolute() and counter_gather.is_dir() and not counter_gather.is_symlink()
if flat_matrix:
    empty_receipt=flat_matrix  # SAME byte endpoint ONLY;distinct code/owner/hash below
if empty_receipt:
    counter_gather=empty_receipt  # SAME captured-byte FIFO endpoint;distinct authenticated program/owner below
tid_boundary='--tid-boundary' in sys.argv
if tid_boundary:
    assert sys.argv.count('--tid-boundary')==1
    sys.argv.remove('--tid-boundary')
packet_identity_boundary='--packet-identity-boundary' in sys.argv
if packet_identity_boundary:
    assert sys.argv.count('--packet-identity-boundary')==1
    sys.argv.remove('--packet-identity-boundary')
cohort_local_sources='--cohort-local-sources' in sys.argv
if cohort_local_sources:
    assert sys.argv.count('--cohort-local-sources')==1
    sys.argv.remove('--cohort-local-sources')
commit_boundary='--commit-boundary-seals' in sys.argv
if commit_boundary:
    assert sys.argv.count('--commit-boundary-seals')==1
    sys.argv.remove('--commit-boundary-seals')
retained_cohort='--retained-conv-closure' in sys.argv
if retained_cohort:
    assert sys.argv.count('--retained-conv-closure')==1
    sys.argv.remove('--retained-conv-closure')
token_frontier='--token-frontier' in sys.argv
if token_frontier:
    assert sys.argv.count('--token-frontier')==1
    sys.argv.remove('--token-frontier')
job_boot_fold='--job-boot-fold' in sys.argv
if job_boot_fold:
    assert sys.argv.count('--job-boot-fold')==1
    sys.argv.remove('--job-boot-fold')
if '--root-instruction-profile' in sys.argv:
    assert sys.argv.count('--root-instruction-profile')==1
    index=sys.argv.index('--root-instruction-profile')
    assert index+1<len(sys.argv)
    root_profile=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert root_profile.is_absolute() and root_profile.is_dir()
precise_beta = '--precise-beta' in sys.argv
if precise_beta:
    assert sys.argv.count('--precise-beta')==1
    sys.argv.remove('--precise-beta')
fault_marker = '--fault-marker' in sys.argv
posted_cancel = '--posted-cancel' in sys.argv
if posted_cancel:
    assert sys.argv.count('--posted-cancel')==1
    sys.argv.remove('--posted-cancel')
if fault_marker:
    assert sys.argv.count('--fault-marker')==1
    sys.argv.remove('--fault-marker')
held_hidden_readout = '--held-hidden-readout' in sys.argv
if held_hidden_readout:
    assert sys.argv.count('--held-hidden-readout') == 1, 'duplicate held hidden TEST selector'
    sys.argv.remove('--held-hidden-readout')
assert '--hold-recheck-pending' not in sys.argv, 'historical pending-PC selector retired; use --root-frontend-snapshots with the original full-END predecessor'
root_frontend_snapshots = '--root-frontend-snapshots' in sys.argv
if root_frontend_snapshots:
    assert sys.argv.count('--root-frontend-snapshots') == 1, 'duplicate ROOT frontend TEST selector'
    sys.argv.remove('--root-frontend-snapshots')
control_operations = '--control-operations' in sys.argv
if control_operations:
    assert sys.argv.count('--control-operations') == 1
    sys.argv.remove('--control-operations')
control_adapter = '--control-adapter' in sys.argv
if control_adapter:
    assert sys.argv.count('--control-adapter') == 1
    sys.argv.remove('--control-adapter')
backend_finish = '--backend-finish' in sys.argv
if backend_finish:
    assert sys.argv.count('--backend-finish') == 1
    sys.argv.remove('--backend-finish')
compile_link_only = '--compile-link-only' in sys.argv
if compile_link_only:
    assert sys.argv.count('--compile-link-only') == 1
    sys.argv.remove('--compile-link-only')
precise_context = '--precise-context' in sys.argv
if precise_context:
    assert sys.argv.count('--precise-context') == 1
    sys.argv.remove('--precise-context')
pushed_context = '--pushed-context' in sys.argv
if pushed_context:
    assert sys.argv.count('--pushed-context') == 1 and not precise_context, 'QTP3 cannot mix with QTP2,including explicit selectors'
    sys.argv.remove('--pushed-context')
if precise_beta:
    assert pushed_context and not precise_context and not fault_marker, 'precise-beta TEST requires its distinct G3 code package,not QTP2/fault-injection aliases'
if fault_marker:
    assert pushed_context and len(sys.argv)==5 and sys.argv[2]=='--execution-fault-marker' \
        and not any((held_hidden_readout,root_frontend_snapshots,control_operations,control_adapter,backend_finish)), 'fault TEST requires separate G3 virgin-engine diagnostic,not model/END/Work qualification'
else:
    assert '--execution-fault-marker' not in sys.argv, 'fault TEST requires explicit separate code selection'
if backend_finish:
    assert pushed_context and len(sys.argv) == 7 and sys.argv[2] in ('--execution-end', '--execution-end-reload'), 'backend Finish TEST requires actual G3 first-layer/cancel/STOP/HOLD/drain predecessor'
backend_init_faults = len(sys.argv) == 3 and sys.argv[2] == '--backend-initialization-faults'
if backend_init_faults:
    assert pushed_context, 'backend initialization-fault TEST requires the explicit G3 profile;NO worker/model launch is qualified'
if control_adapter:
    assert backend_init_faults or len(sys.argv) == 6 and sys.argv[2] == '--execution-backend-first-matrix' or backend_finish and sys.argv[2] == '--execution-end-reload', 'private control TEST requires isolated early-init faults,actual G3 backend first matrix OR exact two-cycle cancelled-first-layer Finish'
if control_operations:
    assert control_adapter and backend_finish and pushed_context and sys.argv[2] == '--execution-end-reload', 'private control operations TEST requires exact two-cycle G3 control-Finish predecessor'
if root_frontend_snapshots:
    assert pushed_context and len(sys.argv) == 7 and sys.argv[2] == '--execution-end' and not control_adapter, 'ROOT frontend TEST requires the explicit G3 original first-layer/cancel/STOP/HOLD/full-END history'
if held_hidden_readout:
    assert root_frontend_snapshots and pushed_context and not backend_finish and not control_adapter \
        and len(sys.argv) == 7 and sys.argv[2] == '--execution-end', 'Held hidden TEST requires the exact G3 ROOT frontend first-layer/cancel/STOP/HOLD/Read0/full-END predecessor'
if root_profile:
    assert precise_beta and posted_cancel and held_hidden_readout and not compile_link_only, 'HOST ROOT profile requires original precise-beta first-layer/full-END execution,not a shortened or compile-only path'
if job_boot_fold:
    assert precise_beta and posted_cancel and held_hidden_readout and root_frontend_snapshots \
        and pushed_context and len(sys.argv)==7 and sys.argv[2]=='--execution-end' \
        and not any((fault_marker,root_profile,backend_finish,control_adapter,control_operations)), \
        'Job boot fold TEST requires its distinct exact precise-beta original first-layer/full-END predecessor;not an old profile or physical/Work alias'
if token_frontier:
    assert job_boot_fold, 'Frontier reader TEST requires the SAME complete job-fold original first-layer/cancel/full-END predecessor'
if retained_cohort:
    assert token_frontier and not compile_link_only, 'Retained-cohort TEST requires the SAME complete frontier original first-layer/cancel/full-END predecessor,not an endpoint-only alias'
if commit_boundary:
    assert retained_cohort, 'Commit-boundary TEST requires its distinct actual firmware and SAME original first-layer/cancel/full-END predecessor;this does NOT execute CommitState or prove whole-model numerics'
if cohort_local_sources:
    assert commit_boundary, 'Cohort-local-source TEST requires its distinct code and SAME complete original first-layer/cancel/STOP/HOLD/full-END predecessor;not old native,whole-model or Work qualification'
if packet_identity_boundary:
    assert cohort_local_sources, 'Packet-identity TEST requires its distinct code and SAME complete original first-layer/cancel/STOP/HOLD/full-END predecessor;not old native,whole-model or Work qualification'
if tid_boundary:
    assert packet_identity_boundary, 'TID boundary TEST requires its distinct code and SAME packet-identity original first-layer/cancel/STOP/HOLD/full-END predecessor;not old native,whole-model or Work qualification'
if counter_gather:
    assert tid_boundary and not compile_link_only and not root_profile and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Counter-gather TEST requires its exact original first-layer/cancel/STOP/HOLD/drain/full-END predecessor and frozen ROOT;not a shortened/old/Work alias'
if prefetch_worker:
    assert flat_matrix and precise_beta and pushed_context and posted_cancel and held_hidden_readout \
        and root_frontend_snapshots and len(sys.argv)==7 and sys.argv[2]=='--execution-end' \
        and pathlib.Path(sys.argv[5])==prefetch_worker and not compile_link_only, \
        'Prefetch native TEST requires its exact ten-section worker and whole original first-layer/cancel/STOP/HOLD/drain/END predecessor'
if batched_prefetch:
    assert prefetch_worker and flat_matrix and counter_gather and not root_profile, \
        'Batched native TEST requires explicit matching ROOT+worker/full original first-layer/cancel/STOP/HOLD/drain/END,not an old or shortened alias'
if twin_batch_prefetch:
    assert prefetch_worker and flat_matrix and counter_gather and not root_profile, \
        'Twin-batch native TEST requires its distinct matching ROOT+worker/full original first-layer/cancel/STOP/HOLD/drain/END,not old batched,shortened or Work aliases'
embedding_modes = ('--execution-embedding', '--execution-embedding-source-loss', '--execution-embedding-readback-loss', '--execution-layer-dispatch')
state_contract = len(sys.argv)==5 and sys.argv[2]=='--state-owner-contract'
state_flow = len(sys.argv)==7 and sys.argv[2]=='--execution-end-state-full-flow'
state_snapshots = state_contract or state_flow
full_end_flow = state_flow or len(sys.argv)==7 and sys.argv[2]=='--execution-end-full-flow'
full_end_contract = state_contract or len(sys.argv)==5 and sys.argv[2]=='--full-end-owner-contract'
full_end_fixture = full_end_flow or full_end_contract
readout_flow = full_end_flow or len(sys.argv) == 7 and sys.argv[2] == '--execution-readout-full-flow'
clock_contract = len(sys.argv)==3 and sys.argv[2]=='--clock-owner-contract'
end_modes = ('--execution-end', '--execution-end-reload', '--execution-end-host-delete1', '--execution-end-host-delete2',
    '--execution-end-host-delete3', '--execution-end-host-generation1', '--execution-end-host-generation2',
    '--execution-end-host-generation3', '--execution-end-unreserve-fault', '--execution-end-unreserve-generation')
host_end = len(sys.argv)==7 and sys.argv[2] in end_modes
pci_close = host_end or len(sys.argv)==7 and sys.argv[2] in ('--execution-pci-close','--execution-pci-generation-fault','--execution-pci-readback-fault')
views_close = pci_close or len(sys.argv)==7 and sys.argv[2] in ('--execution-views-close','--execution-views-delete-fault','--execution-views-generation-fault')
hardware_seal = views_close or len(sys.argv)==7 and sys.argv[2] in ('--execution-hardware-seal','--execution-hardware-seal-tlb-fault')
clock_end = hardware_seal or len(sys.argv)==7 and sys.argv[2] in ('--execution-clock-end','--execution-clock-end-generation-fault',
    '--execution-clock-end-response-fault','--execution-clock-end-unmap-fault')
restore_shared = clock_end or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-shared-tlb','--execution-restore-shared-tlb-fault',
    '--execution-restore-shared-tlb-readback-fault','--execution-restore-shared-tlb-verify-fault')
restore_root = restore_shared or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-root-store','--execution-restore-root-store-fault',
    '--execution-restore-root-store-map-fault','--execution-restore-root-store-config-fault','--execution-restore-root-store-scratch-fault',
    '--execution-restore-root-store-fifo-fault','--execution-restore-root-store-overflow-fault')
restore_peers = restore_root or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-peers','--execution-restore-peers-fault',
    '--execution-restore-peers-map-fault','--execution-restore-peers-config-fault','--execution-restore-peers-scratch-fault',
    '--execution-restore-peers-fifo-fault','--execution-restore-peers-overflow-fault')
restore_dispatch = restore_peers or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-dispatch','--execution-restore-dispatch-fault')
restore_empty = restore_dispatch or len(sys.argv) == 7 and sys.argv[2] in ('--execution-restore-empty','--execution-restore-empty-fault')
readout_empty = restore_empty or len(sys.argv) == 7 and sys.argv[2] == '--execution-readout-empty'
full_flow = readout_flow or len(sys.argv) == 7 and sys.argv[2] == '--execution-full-flow'
hold_layer = readout_empty or len(sys.argv) == 7 and sys.argv[2] == '--execution-hold-first-layer'
cancel_layer = hold_layer or len(sys.argv) == 7 and sys.argv[2] == '--execution-cancel-first-layer'
full_layer = full_flow or cancel_layer or len(sys.argv) == 7 and sys.argv[2] == '--execution-first-layer'
if posted_cancel:
    assert pushed_context and cancel_layer and not fault_marker and not full_flow, 'posted transport TEST requires exact G3 original first-layer/cancel lifecycle,not a new Work or full-flow alias'
backend_first_matrix = len(sys.argv) == 6 and sys.argv[2] == '--execution-backend-first-matrix'
first_matrix = full_layer or backend_first_matrix or len(sys.argv) == 6 and sys.argv[2] == '--execution-first-matrix'
if full_layer:
    prefix = pathlib.Path(sys.argv[6])
    if full_flow:
        assert prefix.is_absolute() and prefix.is_dir() and not prefix.is_symlink() and not any(prefix.iterdir())
    else:
        assert prefix.is_absolute() and prefix.parent.is_dir() and not prefix.exists() and not prefix.is_symlink()
        if sys.argv[2]=='--execution-end-reload':
            for cycle in (1, 2):
                path = pathlib.Path(str(prefix)+'.cycle'+str(cycle)+'.bin')
                assert not path.exists() and not path.is_symlink()
embedding = first_matrix or len(sys.argv) == 4 and sys.argv[2] in embedding_modes
peer_bootstrap = fault_marker or len(sys.argv) == 5 and sys.argv[2] == '--bootstrap-peer-engines'
peer_engines = full_end_contract or peer_bootstrap or first_matrix
if precise_beta:
    assert peer_engines, 'precise-beta TEST requires virgin engines or the original full native predecessor;not an endpoint-only alias'
if precise_context:
    assert peer_engines or sys.argv[2:]==['--preparation-endpoint-contract'], 'QTP2 execution requires actual virgin peer instruction engines;endpoint-only contract executes NO worker/model instructions'
if pushed_context:
    assert backend_init_faults or full_end_contract or peer_bootstrap or first_matrix or sys.argv[2:]==['--preparation-endpoint-contract'], 'QTP3 requires actual virgin engines/full predecessor;early-init fault TEST grants no worker/model execution or cleanup'
terminal_faults = len(sys.argv)==3 and sys.argv[2]=='--terminal-observation-faults'
preparation_faults = len(sys.argv)==3 and sys.argv[2] in ('--preparation-fifo-faults', '--preparation-access-faults')
preparation_endpoints = len(sys.argv)==3 and sys.argv[2]=='--preparation-endpoint-contract'
assert backend_init_faults or full_end_contract or clock_contract or preparation_endpoints or preparation_faults or peer_bootstrap or embedding or terminal_faults or len(sys.argv) in (2,3) and (len(sys.argv) == 2 or sys.argv[2] in ('--initializer-fifo-faults', '--faults', '--late-faults', '--held', '--held-faults', '--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'))
if embedding:
    assert pathlib.Path(sys.argv[3]).is_absolute() and pathlib.Path(sys.argv[3]).is_file()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image = pathlib.Path(sys.argv[1])
assert image.is_absolute() and hashlib.sha256((image/'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
here = pathlib.Path(__file__).resolve().parent
if peer_engines:
    peer_arg = 3 if full_end_contract or peer_bootstrap else 4
    for directory, script in ((pathlib.Path(sys.argv[peer_arg]), here/'run_pioneer_q38_mixed_worker_tests.py'),
                              (pathlib.Path(sys.argv[peer_arg+1]), here.parent/'build_qwen38_token_twin_batch_prefetch_worker.py' if twin_batch_prefetch else here.parent/'build_qwen38_token_batched_prefetch_worker.py' if batched_prefetch else here.parent/'build_qwen38_token_prefetch_worker.py' if prefetch_worker else here.parent/'build_qwen38_precise_beta_worker_image.py' if precise_beta else here.parent/'build_qwen38_pushed_worker_image.py' if pushed_context else here/'run_pioneer_q38_precise_worker_tests.py' if precise_context else here.parent/'build_qwen38_global_worker_image.py')):
        assert directory.is_absolute()
        tree = ast.parse(script.read_text())
        pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets))
        for name, size, sha in pins:
            data = (directory/name).read_bytes()
            assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
if precise_context:
    defines.update(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT='1', PIONEER_Q38_TOKEN_PRECISE_PROGRAMS='1',
                   PIONEER_DRIVER_BUILD_TAG='0x5154324e4f445256ULL')
if pushed_context:
    defines.update(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT='1', PIONEER_Q38_TOKEN_PUSHED_PROGRAMS='1',
                   PIONEER_DRIVER_BUILD_TAG='0x5154334e4f445256ULL')
if precise_beta:
    defines.update(PIONEER_Q38_TOKEN_PRECISE_BETA_PROGRAMS='1',PIONEER_Q38_TOKEN_FAULT_PROGRAMS='1',
                   PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC='1')
if job_boot_fold:
    defines.update(PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_PROGRAMS='1',PIONEER_Q38_TOKEN_JOB_BOOT_FOLD_NATIVE_CONTEXT='1')
if token_frontier:
    defines.update(PIONEER_Q38_TOKEN_FRONTIER_PROGRAMS='1',PIONEER_Q38_TOKEN_FRONTIER_NATIVE_CONTEXT='1')
if retained_cohort:
    defines.update(PIONEER_Q38_TOKEN_RETAINED_COHORT_PROGRAMS='1',PIONEER_Q38_TOKEN_RETAINED_COHORT_NATIVE_CONTEXT='1')
if commit_boundary:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenCommitBoundaryPrograms.h').read_bytes()).hexdigest()=='f17464e86b23df5210830446fa894980deba0f1f9a469fd9676db788190df182'
    defines.update(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS='1',PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT='1')
if cohort_local_sources:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenCohortLocalSourcesPrograms.h').read_bytes()).hexdigest()=='51e7bd06cb11ab01f2b2e55b3492ac4dbfa1aa26639f2a02e9635d9615dbd6bb'
    defines.update(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS='1',PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT='1')
if packet_identity_boundary:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenPacketIdentityBoundaryPrograms.h').read_bytes()).hexdigest()=='de0ae6307f235640a067360de2b3acf5a0e30c7de00b06c27322d4d90c1e3243'
    defines.update(PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_PROGRAMS='1',PIONEER_Q38_TOKEN_PACKET_IDENTITY_BOUNDARY_NATIVE_CONTEXT='1')
if tid_boundary:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenTidBoundaryPrograms.h').read_bytes()).hexdigest()=='229da82175d8a83765d4a0494b345749a92de0a93ec373ffb4ffdd45a9584b1d'
    defines.update(PIONEER_Q38_TOKEN_TID_BOUNDARY_PROGRAMS='1',PIONEER_Q38_TOKEN_TID_BOUNDARY_NATIVE_CONTEXT='1')
if counter_gather:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenCounterGatherPrograms.h').read_bytes()).hexdigest()=='c2f4764197a8a402bf6a60972744adc7dfd641914449ab2abc96ea19e6d9bc63'
    defines.update(PIONEER_Q38_TOKEN_COUNTER_GATHER_PROGRAMS='1',PIONEER_Q38_TOKEN_COUNTER_GATHER_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_COUNTER_GATHER_FIXTURE='1')
if empty_receipt:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenEmptyReceiptPollPrograms.h').read_bytes()).hexdigest()=='a96f194bf1ac5172e29e752c3e1f894a692105b2d74bb8b8100fc5a41ca6becf'
    defines.update(PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_PROGRAMS='1',PIONEER_Q38_TOKEN_EMPTY_RECEIPT_POLL_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_EMPTY_RECEIPT_POLL_FIXTURE='1')
if flat_matrix:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenFlatMatrixPrograms.h').read_bytes()).hexdigest()=='8963e12e1acddf582f0c035db2cb45025d1303fbe9e096e11901b36a0352c780'
    defines.update(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS='1',PIONEER_Q38_TOKEN_FLAT_MATRIX_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_FLAT_MATRIX_FIXTURE='1')
if prefetch_worker:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenPrefetchPrograms.h').read_bytes()).hexdigest()=='021fdff4100dd6f8a6ff8548f9fa961473503cf6c2cd2e0b6adc44053b007567'
    defines.update(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS='1',PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_PREFETCH_FIXTURE='1')
if batched_prefetch or twin_batch_prefetch:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenBatchedPrefetchPrograms.h').read_bytes()).hexdigest()=='9b1d913648714127badbb8888b8e000367cea40f01259b6024a3f0096e996fd9'
    defines.update(PIONEER_Q38_TOKEN_BATCHED_PREFETCH_PROGRAMS='1',PIONEER_Q38_TOKEN_BATCHED_PREFETCH_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_BATCHED_PREFETCH_FIXTURE='1')
if twin_batch_prefetch:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenTwinBatchPrefetchPrograms.h').read_bytes()).hexdigest()=='049613d63e3524be9a5aa1d3ef5f234adf4c61e4142a4d79eaf7e3bd50ae9db1'
    defines.update(PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_PROGRAMS='1',PIONEER_Q38_TOKEN_TWIN_BATCH_PREFETCH_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_TWIN_BATCH_PREFETCH_FIXTURE='1')
if posted_cancel:
    defines.update(PIONEER_Q38_TOKEN_POSTED_CANCEL='1',PIONEER_Q38_TOKEN_FAULT_PROGRAMS='1',
                   PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC='1')
if fault_marker:
    defines.update(PIONEER_Q38_TOKEN_FAULT_PROGRAMS='1',PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC='1',
                   PIONEER_Q38_TOKEN_NATIVE_INJECT_FAULT_MARKER='1',
                   PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE='1',PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE='1')
defines.update({name:'pioneer_token_init_test_'+name for name in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')})
defines.update(TT_PIONEER_EXPERIMENTAL_Q38_AFFINE='1', TT_PIONEER_EXPERIMENTAL_Q38_BLOCK='1')
if clock_contract or preparation_endpoints or preparation_faults or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--held', '--held-faults', '--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE'] = '1'
if clock_contract or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--runtime', '--runtime-pci-capture-fault', '--runtime-faults', '--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE'] = '1'
if clock_contract or terminal_faults or peer_bootstrap or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--bootstrap', '--bootstrap-faults', '--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE'] = '1'
if clock_contract or terminal_faults or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--execution', '--execution-faults', '--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE'] = '1'
if terminal_faults or embedding or len(sys.argv) == 3 and sys.argv[2] in ('--execution-traffic', '--execution-traffic-faults'):
    defines['PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE'] = '1'
if embedding:
    defines['PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE'] = '1'
    defines['PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE'] = '1'
    if first_matrix or sys.argv[2] == '--execution-layer-dispatch':
        defines['PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE'] = '1'
if peer_engines:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE='1', PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE='1')
if first_matrix:
    defines['PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE'] = '1'
if full_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE'] = '1'
if cancel_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE'] = '1'
if hold_layer:
    defines['PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE'] = '1'
if root_frontend_snapshots:
    assert hold_layer and host_end and not full_flow
    defines['PIONEER_Q38_TOKEN_NATIVE_ROOT_FRONTEND_FIXTURE'] = '1'
if held_hidden_readout:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_HIDDEN_READ_FIXTURE='1', PIONEER_Q38_TOKEN_HELD_HIDDEN_DIAGNOSTIC='1')
if root_profile:
    defines['PIONEER_Q38_NATIVE_ROOT_PROFILE_FIXTURE']='1'
if readout_empty:
    defines['PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE'] = '1'
if restore_empty:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE'] = '1'
if restore_dispatch:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE'] = '1'
if restore_peers:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE'] = '1'
if restore_root:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE'] = '1'
if restore_shared:
    defines['PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE'] = '1'
if clock_contract or clock_end or backend_first_matrix:
    defines['PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'] = '1'
if backend_first_matrix:
    assert pushed_context, 'private backend first-matrix qualification selects the actual G3 family explicitly'
    defines['PIONEER_Q38_TOKEN_NATIVE_BACKEND_FIXTURE'] = '1'
if hardware_seal:
    defines['PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE'] = '1'
if views_close:
    defines['PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE'] = '1'
if pci_close:
    defines['PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE'] = '1'
if host_end:
    defines['PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE'] = '1'
if readout_flow:
    defines['PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE'] = '1'
if full_flow:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE='1', PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE='1')
if full_end_fixture:
    defines.update(dict.fromkeys(('PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_READOUT_FLOW_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_TRAFFIC_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_EMBEDDING_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_LAYER_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_FIRST_MATRIX_FIXTURE', 'PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE', 'PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE',
        'PIONEER_Q38_NATIVE_TRACE_TEST_BRIDGE'), '1'))
    assert not any(name in defines for name in ('PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE'))
source = (kernel/'tenstorrent_pci_probe.cpp').read_text()
if state_snapshots:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE='1', PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE='1')
if backend_finish:
    defines['PIONEER_Q38_TOKEN_NATIVE_BACKEND_FINISH_FIXTURE'] = '1'
if backend_init_faults:
    defines.update(dict.fromkeys(('PIONEER_Q38_TOKEN_NATIVE_BACKEND_INIT_FAULT_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_HELD_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_RUNTIME_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_BOOTSTRAP_FIXTURE', 'PIONEER_Q38_TOKEN_NATIVE_EXECUTION_FIXTURE',
        'PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE'), '1'))
if control_adapter:
    defines['PIONEER_Q38_TOKEN_NATIVE_CONTROL_FIXTURE'] = '1'
    if backend_first_matrix:
        defines['PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE'] = '1'
    if backend_finish:
        defines['PIONEER_Q38_TOKEN_NATIVE_CONTROL_FINISH_FIXTURE'] = '1'
    if control_operations:
        defines['PIONEER_Q38_TOKEN_NATIVE_CONTROL_OPERATIONS_FIXTURE'] = '1'
with tempfile.TemporaryDirectory(prefix='pioneer-token-native-init-', dir='/private/tmp') as tmp:
    base = pathlib.Path(tmp)
    start, end = '// PIONEER_Q38_CLOCK_IO_BEGIN', '// PIONEER_Q38_CLOCK_IO_END'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_clock_io.inc').write_text(source.split(start,1)[1].split(end,1)[0])
    session = source[source.index('struct PioneerSessionIO :'):]
    signature = '\tvoid QuarantineHostPage() {'
    assert session.count(signature) == 1
    begin = session.index(signature)
    at = session.index('{', begin)
    depth, end = 1, at+1
    while depth:
        depth += (session[end] == '{')-(session[end] == '}')
        end += 1
    (base/'token_native_host_quarantine.inc').write_text(session[begin:end]+'\n')
    # The session wrapper calls this actual production base method. The
    # old HostReadMock assertion incorrectly assumes a page is still live
    # after every retention failure,including after successful release.
    host_class = source[source.index('struct PioneerHostPageIO {'):source.index('struct PioneerOutputBufferIO {')]
    assert host_class.count(signature)==1
    begin=host_class.index(signature);at=host_class.index('{',begin);depth,end=1,at+1
    while depth:
        depth+=(host_class[end]=='{')-(host_class[end]=='}');end+=1
    (base/'token_native_base_host_quarantine.inc').write_text(host_class[begin:end]+'\n')
    start, end = '\tbool Unsafe(\n', '\n\tvoid Snapshot(tt_resident_session_result& r)'
    assert source.count(start) == source.count(end) == 1
    (base/'token_native_unsafe.inc').write_text(start+source.split(start, 1)[1].split(end, 1)[0])
    for name, marker in (('lock', 'PIONEER_PROBE_LOCK'), ('retain', 'PIONEER_Q38_RETAIN_MEMORY')):
        start, end = '// '+marker+'_BEGIN', '// '+marker+'_END'
        assert source.count(start) == source.count(end) == 1
        (base/('token_native_'+name+'.inc')).write_text(source.split(start, 1)[1].split(end, 1)[0])
    start = 'EndQ38DispatchOwnership(PioneerStoreContext& c,tt_resident_session_result& result,bool& clean)\n{\n'
    assert source.count(start) == 1
    branch = source.split(start, 1)[1].split('\n#endif', 1)[0]+'\n#endif\n'
    assert branch.startswith('#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT\n')
    (base/'token_native_end.inc').write_text(branch)

    def method(signature):
        assert source.count(signature) == 1
        begin = source.index(signature)
        at = source.index('{', begin)
        depth, end = 1, at+1
        while depth:
            depth += (source[end] == '{')-(source[end] == '}')
            end += 1
        return source[begin:end]+'\n'

    (base/'token_native_zero_callbacks.inc').write_text(''.join(method(signature) for signature in
        ('bool PioneerSessionIO::StateZeroBeforeCohort()', 'bool PioneerSessionIO::StateZeroOtherCoresHeld()', 'bool PioneerSessionIO::NonRootHeldOwner()')))
    binary = base/'test'
    extra = []
    profile_environment={}
    if counter_gather:
        # Authenticate ALL frozen artifacts before classifying any TEST
        # request. nm consumes the SAME already authenticated ELF snapshot,
        # not a second unchecked open of the original image pathname.
        tree=ast.parse((here.parent/('build_qwen38_batched_prefetch_root.py' if batched_prefetch or twin_batch_prefetch else 'build_qwen38_flat_matrix_grammar_root.py' if flat_matrix else 'build_qwen38_empty_receipt_poll_root.py' if empty_receipt else 'build_qwen38_counter_gather_root.py')).read_text())
        pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
                  and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
        elf=None
        for name,size,digest in pins:
            data=(counter_gather/name).read_bytes()
            assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
            if name=='program.elf':elf=data
        assert elf is not None
        snapshot=base/'counter-gather-PINNED.elf';snapshot.write_bytes(elf)
        tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
        table=subprocess.check_output([tool,'-S','-nC',str(snapshot)],text=True)
        assert snapshot.read_bytes()==elf
        ranges=[(int(row[0],16),int(row[1],16)) for line in table.splitlines()
                if len(row:=line.split(None,3))==4 and row[2] in ('t','T')
                and row[3].startswith('PioneerQ38CounterGather::Read(')]
        assert len(ranges)==1
        begin,size=ranges[0];assert begin>=0x40000 and size and begin+size<=0x60000 and not (begin|size)&3
        profile_environment['PIONEER_TEST_COUNTER_GATHER_RANGE']=f'{begin:x}:{begin+size:x}'
        print(f'AUTHENTICATED_NATIVE_COUNTER_GATHER_SCOPE {begin:x}..{begin+size:x}/ELF'+hashlib.sha256(elf).hexdigest()+
              ';HOST byte-endpoint classification ONLY,not supplied PC,owner,model result or hardware authority',flush=True)
    if root_profile:
        from pioneer_q38_native_root_profile_map import build_profile_map
        metadata=build_profile_map(root_profile,base)
        print('AUTHENTICATED_HOST_NATIVE_ROOT_PROFILE ELF'+metadata['elf_sha256']+
              '/map'+metadata['map_sha256']+'/ranges'+str(metadata['ranges'])+
              ';READ_ONLY_HOST_PC_OBSERVATIONS_NOT_DEVICE_CYCLES_OR_TPS',flush=True)
        profile_environment['PIONEER_TEST_ROOT_PROFILE_MAP']=str(metadata['path'])
        profile_object=base/'root-profile-host.o'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-DPIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST=1',
                        '-c',str(here/'pioneer_q38_native_root_profile_bridge.cpp'),'-o',str(profile_object)],check=True)
        extra += [str(profile_object)] # HOST-only count sink;NO kernel/native/CPU-model/Work flags
    if state_snapshots:
        state_object = base/'native-state-data.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_native_state_test_bridge.cpp'), '-o', str(state_object)], check=True)
        extra += [str(state_object)]  # HOST-only output DATA;no native/CPU-oracle roles
    if full_flow or full_end_contract:
        trace_object = base/'native-observed-trace.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_native_trace_test_bridge.cpp'), '-o', str(trace_object)], check=True)
        extra += [str(trace_object)]  # const observation DATA only,HOST TU without native/kernel/Work roles
    if peer_engines or full_end_contract:
        peer_object = base/'native-peer-engine.o'
        peer_flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
        if precise_context:
            peer_flags += ['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1']  # DATA only;never native/kernel roles
        if pushed_context:
            peer_flags += ['-DPIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1']  # distinct HOST DATA,NEVER BUILD/native/kernel flags
        if precise_beta:
            # The optimized ROOT has EXACTLY the same nine peer arrays.
            # This bridge observes peers1..119 ONLY;ROOT code comes from
            # the new native package and is independently authenticated.
            peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST=1']
            if twin_batch_prefetch:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST=1']
            elif batched_prefetch:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST=1']
            elif prefetch_worker:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_PREFETCH_TEST=1']
            rom_object=base/'precise-beta-expected-rom-data.o'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-c',str(here/('pioneer_q38_twin_batch_prefetch_program_data_bridge.cpp' if twin_batch_prefetch else 'pioneer_q38_batched_prefetch_program_data_bridge.cpp' if batched_prefetch else 'pioneer_q38_prefetch_program_data_bridge.cpp' if prefetch_worker else 'pioneer_q38_precise_beta_program_data_bridge.cpp')),
                            '-o',str(rom_object)],check=True)
            extra += [str(rom_object)] # DATA TU has NO inherited tensor-Work/native/CPU-oracle roles
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', *peer_flags, '-c', str(here/'pioneer_q38_native_peer_test_bridge.cpp'), '-o', str(peer_object)], check=True)
        extra += [str(peer_object)]  # HOST-only TU;never native/kernel-role flags
    if embedding or full_end_contract:
        # RAW byte transport is a separate TEST-only TU,without native-role
        # defines. Never promote its CPU-validation preprocessor role into
        # the native factory/driver candidate or its SDK/Work build.
        raw_object = base/'original-raw.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(raw_object)], check=True)
        bridge_object = base/'embedding-raw-bridge.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_embedding_raw_test_bridge.cpp'), '-o', str(bridge_object)], check=True)
        extra += [str(raw_object), str(bridge_object)]
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), str(here/'pioneer_q38_token_native_initialization_test.cpp'),
                    *extra, '-o', str(binary)], check=True)
    if compile_link_only:
        print('COMPILE/LINK ONLY: actual native TEST fixture+selected bridge objects linked;binary NEVER executed,NO initialization/RUN/STOP/END/model/physical/TPS qualification')
    else:
        if state_flow:
            # Refused arguments/roles and failed builds must publish nothing.
            # This output directory is not a state/completion/native receipt.
            (prefix/'state-native').mkdir(mode=0o700)
        subprocess.run([str(binary), *sys.argv[1:]], check=True,
                       env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1',**profile_environment})
