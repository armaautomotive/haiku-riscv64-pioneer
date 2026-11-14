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
ffn_reuse_header='--recurrent-ffn-reuse-header' in sys.argv
if ffn_reuse_header:
    assert sys.argv.count('--recurrent-ffn-reuse-header')==1 and '--recurrent-native-header' not in sys.argv
    sys.argv[sys.argv.index('--recurrent-ffn-reuse-header')]='--recurrent-native-header'
recurrent_final='--recurrent-final' in sys.argv
if recurrent_final:
    assert sys.argv.count('--recurrent-final')==1
    sys.argv.remove('--recurrent-final')
ffn_down='--recurrent-ffn-down' in sys.argv
if ffn_down:
    assert sys.argv.count('--recurrent-ffn-down')==1
    sys.argv.remove('--recurrent-ffn-down')
ffn_activation='--recurrent-ffn-activation' in sys.argv
if ffn_activation:
    assert sys.argv.count('--recurrent-ffn-activation')==1
    sys.argv.remove('--recurrent-ffn-activation')
ffn_up='--recurrent-ffn-up' in sys.argv
if ffn_up:
    assert sys.argv.count('--recurrent-ffn-up')==1
    sys.argv.remove('--recurrent-ffn-up')
ffn_matrix='--recurrent-ffn-matrix' in sys.argv
if ffn_matrix:
    assert sys.argv.count('--recurrent-ffn-matrix')==1
    sys.argv.remove('--recurrent-ffn-matrix')
ffn_producers='--recurrent-ffn-producers' in sys.argv
if ffn_producers:
    assert sys.argv.count('--recurrent-ffn-producers')==1
    sys.argv.remove('--recurrent-ffn-producers')
output_matrix='--recurrent-output-matrix' in sys.argv
if output_matrix:
    assert sys.argv.count('--recurrent-output-matrix')==1
    sys.argv.remove('--recurrent-output-matrix')
output_producers='--recurrent-output-producers' in sys.argv
if output_producers:
    assert sys.argv.count('--recurrent-output-producers')==1
    sys.argv.remove('--recurrent-output-producers')
gdn_cohort='--recurrent-gdn-cohort' in sys.argv
if gdn_cohort:
    assert sys.argv.count('--recurrent-gdn-cohort')==1
    sys.argv.remove('--recurrent-gdn-cohort')
conv_cohort='--recurrent-conv-cohort' in sys.argv
if conv_cohort:
    assert sys.argv.count('--recurrent-conv-cohort')==1
    sys.argv.remove('--recurrent-conv-cohort')
fault_options=('--qkv-conv-receipt-fault','--qkv-conv-job-fault','--qkv-conv-counter-fault')
selected_faults=[option for option in fault_options if option in sys.argv]
assert len(selected_faults)<=1
qkv_conv_fault=1+fault_options.index(selected_faults[0]) if selected_faults else 0
if qkv_conv_fault:
    assert sys.argv.count(selected_faults[0])==1
    sys.argv.remove(selected_faults[0])
qkv_cohort='--recurrent-qkv-cohort' in sys.argv
if qkv_cohort:
    assert sys.argv.count('--recurrent-qkv-cohort')==1
    sys.argv.remove('--recurrent-qkv-cohort')
gate_cohort='--recurrent-gate-cohort' in sys.argv
if gate_cohort:
    assert sys.argv.count('--recurrent-gate-cohort')==1
    sys.argv.remove('--recurrent-gate-cohort')
small_to_gate='--recurrent-small-to-gate' in sys.argv
if small_to_gate:
    assert sys.argv.count('--recurrent-small-to-gate')==1
    sys.argv.remove('--recurrent-small-to-gate')
model_recurrent=None
model_output=None
if '--model-output-fixture' in sys.argv:
    i=sys.argv.index('--model-output-fixture');assert sys.argv.count('--model-output-fixture')==1 and i+1<len(sys.argv)
    model_output=pathlib.Path(sys.argv[i+1]);del sys.argv[i:i+2]
    assert model_output.is_absolute() and model_output.is_file() and not model_output.is_symlink()
    assert hashlib.sha256(model_output.read_bytes()).hexdigest()=='8158c4fbaa3569ae7f3453d367b7d988a6d67d0107817c7af524308f1814d886'
model_gate=None
if '--model-gate-fixture' in sys.argv:
    i=sys.argv.index('--model-gate-fixture');assert sys.argv.count('--model-gate-fixture')==1 and i+1<len(sys.argv)
    model_gate=pathlib.Path(sys.argv[i+1]);del sys.argv[i:i+2]
    assert model_gate.is_absolute() and model_gate.is_file() and not model_gate.is_symlink()
    assert hashlib.sha256(model_gate.read_bytes()).hexdigest()=='e3c1189211445020054aeb42ccd89e0d968fb39acd35d0be99ff64755fb1f02b'
model_qkv=None
if '--model-qkv-fixture' in sys.argv:
    i=sys.argv.index('--model-qkv-fixture');assert sys.argv.count('--model-qkv-fixture')==1 and i+1<len(sys.argv)
    model_qkv=pathlib.Path(sys.argv[i+1]);del sys.argv[i:i+2]
    assert model_qkv.is_absolute() and model_qkv.is_file() and not model_qkv.is_symlink()
    assert hashlib.sha256(model_qkv.read_bytes()).hexdigest()=='6146121bb87b7a684d544b73e06a62f0a6b05513866715bfdbff3dfc840d514a'
if '--model-recurrent-fixtures' in sys.argv:
    i=sys.argv.index('--model-recurrent-fixtures');assert sys.argv.count('--model-recurrent-fixtures')==1 and i+2<len(sys.argv)
    model_recurrent=[pathlib.Path(p) for p in sys.argv[i+1:i+3]];del sys.argv[i:i+3]
    for p,digest in zip(model_recurrent,('92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb','f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753')):
        assert p.is_absolute() and p.is_file() and not p.is_symlink() and hashlib.sha256(p.read_bytes()).hexdigest()==digest
recurrent_matrix_reuse='--recurrent-matrix-reuse' in sys.argv
if recurrent_matrix_reuse:
    assert sys.argv.count('--recurrent-matrix-reuse')==1
    sys.argv.remove('--recurrent-matrix-reuse')
synthetic_recurrent_matrix='--synthetic-recurrent-matrix' in sys.argv
if synthetic_recurrent_matrix:
    assert sys.argv.count('--synthetic-recurrent-matrix')==1
    sys.argv.remove('--synthetic-recurrent-matrix')
trace_init='--qgw8-trace-init' in sys.argv
if trace_init:
    assert sys.argv.count('--qgw8-trace-init')==1
    sys.argv.remove('--qgw8-trace-init')
synthetic_embedding='--synthetic-embedding' in sys.argv
if synthetic_embedding:
    assert sys.argv.count('--synthetic-embedding')==1
    sys.argv.remove('--synthetic-embedding')
recurrent_matrix_cohort='--execution-recurrent-matrix-cohort' in sys.argv and '--recurrent-native-header' in sys.argv
assert not recurrent_matrix_reuse or recurrent_matrix_cohort, 'reuse test requires the isolated matrix cohort'
assert not model_recurrent or recurrent_matrix_reuse, 'model-weight fixtures require the isolated recurrent reuse test'
assert not small_to_gate or recurrent_matrix_reuse, 'small-to-gate check requires the isolated recurrent reuse test'
assert not gate_cohort or small_to_gate, 'gate cohort requires the isolated small-to-gate test'
assert not qkv_cohort or gate_cohort, 'QKV cohort requires the isolated gate cohort test'
assert not model_qkv or (model_recurrent and qkv_cohort), 'real QKV requires the isolated model recurrent QKV cohort'
assert not model_gate or (model_qkv and gate_cohort), 'real gate requires the isolated real QKV and gate cohort'
assert not conv_cohort or qkv_cohort, 'Conv cohort requires the isolated QKV cohort test'
assert not gdn_cohort or conv_cohort, 'GDN test requires the isolated Conv cohort test'
assert not output_producers or gdn_cohort, 'output producers require the isolated GDN cohort test'
assert not output_matrix or output_producers, 'output matrix requires isolated output producers'
assert not model_output or (model_gate and output_matrix), 'real output projection requires real gate and output cohort'
assert not ffn_producers or output_matrix, 'FFN producers require the isolated output matrix'
assert not ffn_matrix or ffn_producers, 'FFN matrix requires the isolated FFN producers'
assert not ffn_up or ffn_matrix, 'FFN up matrix requires the isolated first FFN matrix'
assert not ffn_activation or ffn_up, 'FFN activation requires the isolated FFN up matrix'
assert not ffn_down or ffn_activation, 'FFN down matrix requires the isolated FFN activation'
assert not recurrent_final or ffn_down, 'recurrent final requires the isolated FFN down matrix'
assert not ffn_reuse_header or ffn_up, 'FFN reuse inspection requires the isolated FFN up matrix'
assert not qkv_conv_fault or qkv_cohort, 'receipt fault requires the isolated QKV cohort test'
recurrent_matrix_peer=('--execution-recurrent-matrix' in sys.argv or recurrent_matrix_cohort) and '--recurrent-native-header' in sys.argv
recurrent_producers=('--execution-recurrent-producers' in sys.argv or recurrent_matrix_peer) and '--recurrent-native-header' in sys.argv
recurrent_producer=('--execution-recurrent-producer' in sys.argv or recurrent_producers) and '--recurrent-native-header' in sys.argv
assert not synthetic_recurrent_matrix or recurrent_producers and synthetic_embedding, 'synthetic recurrent matrix data requires its isolated producer cohort'
assert not recurrent_matrix_peer or synthetic_recurrent_matrix, 'recurrent matrix peer requires synthetic test weights'
recurrent_layer_peer=('--execution-recurrent-layer-peer' in sys.argv or recurrent_producer) and '--recurrent-native-header' in sys.argv
synthetic_layer=synthetic_embedding and ('--execution-layer-dispatch' in sys.argv or recurrent_layer_peer)
recurrent_peer=('--bootstrap-peer-engines' in sys.argv or recurrent_layer_peer) and '--recurrent-native-header' in sys.argv
hybrid_header=None
recurrent_inspection='--recurrent-native-header' in sys.argv
qgwf_unroll_inspection='--qgwf-unroll-frontier-header' in sys.argv
qgwf_frontier_inspection=qgwf_unroll_inspection or '--qgwf-frontier-header' in sys.argv
qgwf_inspection=recurrent_inspection or qgwf_frontier_inspection or '--qgwf-header' in sys.argv
qgwe_inspection=qgwf_inspection or '--qgwe-header' in sys.argv
qgwd_seal_inspection='--qgwd-seal-header' in sys.argv
qgwd_inspection='--qgwd-header' in sys.argv or qgwd_seal_inspection or qgwe_inspection
qgw8_inspection='--qgw8-header' in sys.argv or qgwd_inspection
matrix_inspection='--hybrid-qgw7-header' in sys.argv or qgw8_inspection
if '--hybrid-qgw5-header' in sys.argv or matrix_inspection:
    selectors=('--recurrent-native-header','--qgwf-unroll-frontier-header','--qgwf-frontier-header','--qgwf-header','--qgwe-header','--qgwd-seal-header','--qgwd-header','--qgw8-header','--hybrid-qgw7-header','--hybrid-qgw5-header')
    assert sum(sys.argv.count(s) for s in selectors)==1
    selector='--recurrent-native-header' if recurrent_inspection else '--qgwf-unroll-frontier-header' if qgwf_unroll_inspection else '--qgwf-frontier-header' if qgwf_frontier_inspection else '--qgwf-header' if qgwf_inspection else '--qgwe-header' if qgwe_inspection else '--qgwd-seal-header' if qgwd_seal_inspection else '--qgwd-header' if qgwd_inspection else '--qgw8-header' if qgw8_inspection else '--hybrid-qgw7-header' if matrix_inspection else '--hybrid-qgw5-header'
    index=sys.argv.index(selector);assert index+1<len(sys.argv)
    hybrid_header=pathlib.Path(sys.argv[index+1]);del sys.argv[index:index+2]
    assert (len(sys.argv)==2 or len(sys.argv)==3 and (sys.argv[2] in ('--held','--runtime','--bootstrap','--execution','--execution-traffic')
            or qgw8_inspection and (sys.argv[2]=='--faults' or trace_init and sys.argv[2]=='--runtime-faults'))
            or synthetic_embedding and len(sys.argv)==4 and sys.argv[2] in ('--execution-embedding','--execution-layer-dispatch')
            or recurrent_layer_peer and synthetic_embedding and len(sys.argv)==6
            or recurrent_peer and len(sys.argv)==5 and sys.argv[2]=='--bootstrap-peer-engines') and hybrid_header.is_absolute() and hybrid_header.is_file()
    if qgw8_inspection:
        assert (not synthetic_embedding and (len(sys.argv)==2 or len(sys.argv)==3 and (sys.argv[2] in ('--held','--faults','--runtime','--bootstrap','--execution','--execution-traffic') or trace_init and sys.argv[2]=='--runtime-faults'))
                or synthetic_embedding and len(sys.argv)==4 and sys.argv[2] in ('--execution-embedding','--execution-layer-dispatch')
                or recurrent_layer_peer and synthetic_embedding and len(sys.argv)==6
                or recurrent_peer and len(sys.argv)==5 and sys.argv[2]=='--bootstrap-peer-engines'), \
            'QGW8 profile is limited to early execution and explicit synthetic embedding inspection'
    expected='0c3c8a420ae424cbda4137a41cbf684e45557c9663bfa78f11d1affb6756fcc8' if ffn_reuse_header else '8380e513a2af99644130f293b8a3239109ec42654daa950db482e2a3b20c5b82' if recurrent_inspection else '6455f12643172c1fa70df9271025f1b634011f5ea9c12612ec2e7fcbf3f9edd8' if qgwf_unroll_inspection else '7064d728017fe418f66d97f8e369fee772d730ef208a4a127d72c39e4222aa80' if qgwf_frontier_inspection else 'b0c3d213676ded8ec484cee54908fc6443d4f2f50e199084a782230111fc1ac6' if qgwf_inspection else '4377811ebdc782bb67251beb120e32fdef18aab29ece5e8d77ba07bbcb0846b2' if qgwe_inspection else '8888219277aefb5a5c2a1cd1e60f3240c661bc839f7485d8965c8aa767843742' if qgwd_seal_inspection else '90dd22aada31d665a42fda9fa158afd888d13e04c45365203ae5f42c0fb6daff' if qgwd_inspection else '72ec049c6905e37531b4fc21854991a8ddf513fc0944e8acde2ed73fc59a8588' if qgw8_inspection else '5dd198d4676599329fd59626a8924ef2f8fbe4a905883a471f6294fa89efe7b7' if matrix_inspection else '2598f7a469ddd843ab1bbe732d0960dacaed819b4710b507430274b46c874f49'
    assert hashlib.sha256(hybrid_header.read_bytes()).hexdigest()==expected
if trace_init:
    assert qgw8_inspection and len(sys.argv)==3 and sys.argv[2] in ('--runtime','--runtime-faults')
manual_ttsync='--manual-ttsync' in sys.argv
if manual_ttsync:
    assert sys.argv.count('--manual-ttsync')==1
    sys.argv.remove('--manual-ttsync')
manual_watchdog='--manual-watchdog' in sys.argv
if manual_watchdog:
    assert manual_ttsync and sys.argv.count('--manual-watchdog')==1
    sys.argv.remove('--manual-watchdog')
matrix_phase_read='--matrix-phase-held-readout' in sys.argv
if matrix_phase_read:
    assert sys.argv.count('--matrix-phase-held-readout')==1
    sys.argv.remove('--matrix-phase-held-readout')
matrix_phase='--token-matrix-phase' in sys.argv
if matrix_phase:
    assert sys.argv.count('--token-matrix-phase')==1
    sys.argv.remove('--token-matrix-phase')
constant_q4='--constant-q4-issue' in sys.argv
if constant_q4:
    assert sys.argv.count('--constant-q4-issue')==1
    sys.argv.remove('--constant-q4-issue')
token_phase_profile='--token-phase-profile' in sys.argv
if token_phase_profile:
    assert sys.argv.count('--token-phase-profile')==1
    sys.argv.remove('--token-phase-profile')
wide_gdn='--wide-gdn' in sys.argv
if wide_gdn:
    assert sys.argv.count('--wide-gdn')==1
    sys.argv.remove('--wide-gdn')
output_closure='--batched-output-closure' in sys.argv
if output_closure:
    assert sys.argv.count('--batched-output-closure')==1
    sys.argv.remove('--batched-output-closure')
frontier_failure_link='--frontier-failure-link' in sys.argv
if frontier_failure_link:
    assert sys.argv.count('--frontier-failure-link')==1
    sys.argv.remove('--frontier-failure-link')
frontier_failure_detail='--frontier-failure-detail' in sys.argv
if frontier_failure_detail:
    assert sys.argv.count('--frontier-failure-detail')==1
    sys.argv.remove('--frontier-failure-detail')
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
if frontier_failure_detail:
    assert twin_batch_prefetch and token_frontier and not batched_prefetch and not compile_link_only \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Failure-detail native TEST requires the exact complete twin original-model first-layer/cancel/STOP/HOLD/drain/END history;not compile-only,old,shortened,full-flow or Work aliases'
if frontier_failure_link:
    assert frontier_failure_detail and twin_batch_prefetch and not compile_link_only \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Failure-link native TEST requires explicit QFD1 parents and the exact full original twin first-layer/cancel/STOP/HOLD/drain/END history'
if output_closure:
    assert frontier_failure_link and frontier_failure_detail and twin_batch_prefetch and flat_matrix \
        and not any((batched_prefetch,compile_link_only,root_profile,backend_finish,control_adapter,control_operations)) \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Corrected-output native TEST requires its distinct complete original QFL1/twin first-layer/cancel/STOP/HOLD/drain/END history'
if wide_gdn:
    assert output_closure and not compile_link_only and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Wide GDN native TEST requires own frozen full worker and complete corrected-output original-model first-layer/cancel/STOP/HOLD/drain/END history'
if token_phase_profile:
    assert wide_gdn and output_closure and not compile_link_only and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Full-token timing TEST requires own complete ROOT and wide-GDN original first-layer/cancel/STOP/HOLD/drain/END history;not old,shortened,Work or physical timing'
if constant_q4:
    assert token_phase_profile and wide_gdn and output_closure and not compile_link_only \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Constant Q4 TEST requires its exact new worker and complete timing original first-layer/cancel/STOP/HOLD/drain/END predecessor;not old,shortened,Work or hardware timing'
if matrix_phase:
    assert constant_q4 and token_phase_profile and not compile_link_only and not root_profile \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Sampler TEST requires own whole new worker and SAME original-model first-layer/cancel/STOP/HOLD/drain/full-END history'
if matrix_phase_read:
    assert matrix_phase and constant_q4 and not compile_link_only and not root_profile \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Fresh-slot reader TEST requires SAME exact sampler original-model first-layer/cancel/HOLD/drain/full-END,not an old or shortened alias'
embedding_modes = ('--execution-embedding', '--execution-embedding-source-loss', '--execution-embedding-readback-loss', '--execution-layer-dispatch')
if manual_ttsync:
    assert constant_q4 and token_phase_profile and not matrix_phase and not compile_link_only and not root_profile \
        and len(sys.argv)==7 and sys.argv[2]=='--execution-end', \
        'Manual completion TEST requires its own full worker and SAME original first-layer/cancel/STOP/HOLD/drain/full-END history'
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
embedding = first_matrix or recurrent_layer_peer or len(sys.argv) == 4 and sys.argv[2] in embedding_modes
peer_bootstrap = fault_marker or len(sys.argv) == 5 and sys.argv[2] == '--bootstrap-peer-engines'
peer_engines = full_end_contract or peer_bootstrap or first_matrix or recurrent_layer_peer
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
if synthetic_embedding:
    assert len(sys.argv)==4 and sys.argv[2] in ('--execution-embedding','--execution-layer-dispatch') or recurrent_layer_peer and len(sys.argv)==6
    assert not synthetic_layer or qgw8_inspection, 'synthetic layer weights require the isolated QGW8 profile'
    fixture=pathlib.Path(sys.argv[3]);assert fixture.is_absolute() and fixture.is_file()
    assert fixture.read_bytes()==(b'SYNTHETIC_Q4K_QGW8_LAYER_V1\n' if synthetic_layer else b'SYNTHETIC_Q4K_EMBEDDING_V1\n')
image = pathlib.Path(sys.argv[1])
assert image.is_absolute() and hashlib.sha256((image/'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
here = pathlib.Path(__file__).resolve().parent
if peer_engines:
    peer_arg = 3 if full_end_contract or peer_bootstrap else 4
    for directory, script in ((pathlib.Path(sys.argv[peer_arg]), here/'run_pioneer_q38_mixed_worker_tests.py'),
                              (pathlib.Path(sys.argv[peer_arg+1]), here.parent/'build_qwen38_manual_ttsync_worker.py' if manual_ttsync else here.parent/'build_qwen38_token_matrix_phase_worker.py' if matrix_phase else here.parent/'build_qwen38_q4_constant_issue_worker_image.py' if constant_q4 else here.parent/'build_qwen38_wide_gdn_worker_image.py' if wide_gdn else here.parent/'build_qwen38_token_twin_batch_prefetch_worker.py' if twin_batch_prefetch else here.parent/'build_qwen38_token_batched_prefetch_worker.py' if batched_prefetch else here.parent/'build_qwen38_token_prefetch_worker.py' if prefetch_worker else here.parent/'build_qwen38_precise_beta_worker_image.py' if precise_beta else here.parent/'build_qwen38_pushed_worker_image.py' if pushed_context else here/'run_pioneer_q38_precise_worker_tests.py' if precise_context else here.parent/'build_qwen38_global_worker_image.py')):
        assert directory.is_absolute()
        recurrent_worker=recurrent_peer and directory==pathlib.Path(sys.argv[peer_arg+1])
        tree = ast.parse((here.parent/'PioneerQ38RecurrentProgramPins.py' if recurrent_worker else script).read_text())
        pin_name='WORKER_PINS' if recurrent_worker else 'PINS'
        pins = next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == pin_name for t in n.targets))
        for name, size, sha in pins:
            data = (directory/name).read_bytes()
            assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
        if recurrent_worker:
            assert hashlib.sha256((directory/'program.symbols').read_bytes()).hexdigest()=='2c1531004c53c92642d5add4abfb622827c8170e2c57e2db8bdc9e6c00ff2ec9'
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
role = (kernel/'PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21
defines = dict.fromkeys(parents, '1')
defines['PIONEER_DRIVER_BUILD_TAG'] = '0x51544b4e4f445256ULL'
if hybrid_header:
    defines.update(PIONEER_Q38_HYBRID_TRACKED_UPLOAD='1',PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY='1',
                   PIONEER_Q38_TOKEN_BF16_PROGRAMS='1',PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC='1',
                   PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC='1',PIONEER_Q38_HYBRID_PROGRAMS_HEADER='"'+str(hybrid_header)+'"')
if matrix_inspection:defines.update(PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION='1',PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT='1')
if qgw8_inspection:defines.update(PIONEER_Q38_QGW8_NATIVE_INSPECTION='1',PIONEER_Q38_QGW8_LAYOUT_INSPECTION='1',
                                  PIONEER_Q38_QGW8_PROGRAMS_INSPECTION='1',PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION='1')
if qgwd_inspection:
    assert (len(sys.argv)==3 and (sys.argv[2] in ('--held','--runtime','--bootstrap','--execution')
                                or recurrent_inspection and sys.argv[2]=='--execution-traffic')
            or recurrent_inspection and synthetic_embedding and len(sys.argv)==4
            and sys.argv[2] in ('--execution-embedding','--execution-layer-dispatch')
            or recurrent_layer_peer and synthetic_embedding and len(sys.argv)==6
            or recurrent_peer and len(sys.argv)==5 and sys.argv[2]=='--bootstrap-peer-engines'), \
        'QGWD fixture restricted to startup and explicit synthetic recurrent embedding/layer-dispatch inspection'
    del defines['PIONEER_Q38_QGW8_PROGRAMS_INSPECTION']
    for name in ('PIONEER_Q38_QGWD_NATIVE_INSPECTION','PIONEER_Q38_QGWD_PROGRAMS_INSPECTION',
                 'PIONEER_Q38_QGWD_PLAN_SIMULATION','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_MATRIX_FOOTPRINT_INSPECTION','PIONEER_Q38_QGWD_QGW8_JOURNAL_FIXTURE',
                 'PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION','PIONEER_Q38_QGWD_CONTROL_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION','PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION',
                 'PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION','PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION'):
        defines[name]='1'
    if qgwd_seal_inspection:defines['PIONEER_Q38_QGWD_SEAL_LOOP_INSPECTION']='1'
    if qgwe_inspection:defines.update(PIONEER_Q38_QGWE_NATIVE_INSPECTION='1',PIONEER_Q38_QGWE_PROGRAMS_INSPECTION='1',PIONEER_Q38_QGWE_BATCH_DENSE_PLAN='1')
    if qgwf_inspection:defines.update(PIONEER_Q38_QGWF_NATIVE_INSPECTION='1',PIONEER_Q38_QGWF_PROGRAMS_INSPECTION='1',PIONEER_Q38_QGWF_DUAL_BATCH_PLAN='1')
    if recurrent_inspection:defines.update(PIONEER_Q38_RECURRENT_KERNEL_COMPILE_ONLY='1',PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION='1',PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION='1',PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION='1')
    if recurrent_peer:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PEER_FIXTURE']='1'
    if recurrent_producer:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCER_FIXTURE']='1'
    if recurrent_producers:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_PRODUCERS_FIXTURE']='1'
    if synthetic_recurrent_matrix:defines['PIONEER_Q38_TOKEN_NATIVE_SYNTHETIC_RECURRENT_MATRIX_FIXTURE']='1'
    if recurrent_matrix_peer:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_FIXTURE']='1'
    if recurrent_matrix_cohort:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_COHORT_FIXTURE']='1'
    if recurrent_matrix_reuse:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_MATRIX_REUSE_FIXTURE']='1'
    if small_to_gate:defines['PIONEER_Q38_TOKEN_NATIVE_SMALL_TO_GATE_FIXTURE']='1'
    if gate_cohort:defines['PIONEER_Q38_TOKEN_NATIVE_GATE_COHORT_FIXTURE']='1'
    if qkv_cohort:defines['PIONEER_Q38_TOKEN_NATIVE_QKV_COHORT_FIXTURE']='1'
    if conv_cohort:
        defines['PIONEER_Q38_TOKEN_NATIVE_CONV_COHORT_FIXTURE']='1'
        defines['PIONEER_Q38_SYNTHETIC_CONV_STATE_TEST']='1'
    if gdn_cohort:defines['PIONEER_Q38_TOKEN_NATIVE_GDN_COHORT_FIXTURE']='1'
    if output_producers:defines['PIONEER_Q38_TOKEN_NATIVE_OUTPUT_PRODUCERS_FIXTURE']='1'
    if output_matrix:defines['PIONEER_Q38_TOKEN_NATIVE_OUTPUT_MATRIX_FIXTURE']='1'
    if ffn_producers:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_PRODUCERS_FIXTURE']='1'
    if ffn_matrix:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_MATRIX_FIXTURE']='1'
    if ffn_up:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_UP_FIXTURE']='1'
    if ffn_reuse_header:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_REUSE_FIXTURE']='1'
    if ffn_activation:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_ACTIVATION_FIXTURE']='1'
    if ffn_down:defines['PIONEER_Q38_TOKEN_NATIVE_FFN_DOWN_FIXTURE']='1'
    if recurrent_final:defines['PIONEER_Q38_TOKEN_NATIVE_RECURRENT_FINAL_FIXTURE']='1'
    if qkv_conv_fault:
        defines['PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_RECEIPT_FAULT_FIXTURE']='1'
        defines['PIONEER_Q38_TOKEN_NATIVE_QKV_CONV_FAULT_KIND']=str(qkv_conv_fault)
    if model_recurrent:defines['PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST']='1'
    if model_qkv:defines['PIONEER_Q38_MODEL_QKV_OBSERVER_TEST']='1'
    if model_gate:defines['PIONEER_Q38_MODEL_GATE_OBSERVER_TEST']='1'
    if model_output:defines['PIONEER_Q38_MODEL_OUTPUT_OBSERVER_TEST']='1'
    if qgwf_frontier_inspection:defines.update(PIONEER_Q38_QGWF_FRONTIER_NATIVE_INSPECTION='1',PIONEER_Q38_QGWF_FRONTIER_PROGRAMS_INSPECTION='1')
    if qgwf_unroll_inspection:defines['PIONEER_Q38_QGWF_UNROLL_NATIVE_INSPECTION']='1'
if trace_init:defines.update(PIONEER_Q38_QGW8_TRACE_READ_INSPECTION='1',PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION='1')
if synthetic_embedding:defines['PIONEER_Q38_SYNTHETIC_EMBEDDING_TEST']='1'
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
if frontier_failure_detail:
    defines.update(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_DETAIL_NATIVE='1',
                   PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_DETAIL_FIXTURE='1')
if frontier_failure_link:
    defines.update(PIONEER_Q38_TOKEN_FRONTIER_FAILURE_LINK_NATIVE='1',
                   PIONEER_Q38_TOKEN_NATIVE_FRONTIER_FAILURE_LINK_FIXTURE='1')
if output_closure:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenBatchedOutputClosurePrograms.h').read_bytes()).hexdigest()=='1e844aca55fd4920a278fcd702a64b324fe13ea2c29e52d4cd75841e42f06891'
    defines.update(PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_PROGRAMS='1',
                   PIONEER_Q38_TOKEN_BATCHED_OUTPUT_CLOSURE_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_BATCHED_OUTPUT_CLOSURE_FIXTURE='1')
if wide_gdn:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenWideGdnPrograms.h').read_bytes()).hexdigest()=='d285c9adb421fc0010d8ad48371eb58ca1480fc0d787f7c147afd4a93457a787'
    defines.update(PIONEER_Q38_TOKEN_WIDE_GDN_PROGRAMS='1',PIONEER_Q38_TOKEN_WIDE_GDN_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_WIDE_GDN_FIXTURE='1')
if token_phase_profile:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenPhaseProfilePrograms.h').read_bytes()).hexdigest()=='2fa11c765f2bf6e87dae30973432d83a9b257cefad870fdc65f15ccb294551fe'
    defines.update(PIONEER_Q38_TOKEN_PHASE_PROFILE_PROGRAMS='1',PIONEER_Q38_TOKEN_PHASE_PROFILE_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_PHASE_PROFILE_FIXTURE='1')
if constant_q4:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenConstantQ4Programs.h').read_bytes()).hexdigest()=='79c3056dfbe9b7dd564dc93c3577d72c0bcb004d5018dd0d6b7da212dfad8061'
    defines.update(PIONEER_Q38_TOKEN_CONSTANT_Q4_PROGRAMS='1',PIONEER_Q38_TOKEN_CONSTANT_Q4_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_CONSTANT_Q4_FIXTURE='1')
if matrix_phase:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenMatrixPhasePrograms.h').read_bytes()).hexdigest()=='e42b21bd0a5c0f061f3bbbd2cb620e988d03f561b62fdf444624959ddb03ca09'
    defines.update(PIONEER_Q38_TOKEN_MATRIX_PHASE_PROGRAMS='1',PIONEER_Q38_TOKEN_MATRIX_PHASE_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_FIXTURE='1')
if manual_ttsync:
    assert hashlib.sha256((here.parent/'PioneerQ38TokenManualTTSyncPrograms.h').read_bytes()).hexdigest()=='eaaa4974db72e416f13b2bdd66a4eb5bcdf4d4243a27c990037eea022e009dd1'
    defines.update(PIONEER_Q38_TOKEN_MANUAL_TTSYNC_PROGRAMS='1',PIONEER_Q38_TOKEN_MANUAL_TTSYNC_NATIVE_CONTEXT='1',
                   PIONEER_Q38_TOKEN_NATIVE_MANUAL_TTSYNC_FIXTURE='1')
if manual_watchdog:
    defines.update(PIONEER_Q38_TOKEN_NATIVE_MANUAL_WATCHDOG_FIXTURE='1',PIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST='1')
if matrix_phase_read:
    defines.update(PIONEER_Q38_TOKEN_MATRIX_PHASE_READ_NATIVE='1',PIONEER_Q38_TOKEN_NATIVE_MATRIX_PHASE_READ_FIXTURE='1')
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
    if first_matrix or recurrent_layer_peer or sys.argv[2] == '--execution-layer-dispatch':
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
    if manual_watchdog:
        begin='// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_BEGIN'
        end='// PIONEER_Q38_DISPATCH_COOKIE_RELEASE_END'
        assert source.count(begin)==source.count(end)==1
        (base/'token_native_watchdog_release.inc').write_text(source.split(begin,1)[1].split(end,1)[0])
        caller=(here.parent/'pioneer_q38_frontier_benchmark.cpp').read_text()
        begin=' const bigtime_t began=system_time();bigtime_t previous=began;'
        end=' auto records=std::make_unique<uint32_t[]>(V::Positions*64);'
        assert caller.count(begin)==caller.count(end)==1
        loop=begin+caller.split(begin,1)[1].split(end,1)[0]
        assert 'before-began>=INT64_C(1800000000) || polls>=18000' in loop
        (base/'manual_watchdog_actual_caller.inc').write_text(loop)

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
    if manual_watchdog:
        watchdog_object=base/'manual-caller-watchdog.o'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-DPIONEER_Q38_MANUAL_WATCHDOG_HOST_TEST=1','-I'+str(base),
                        '-c',str(here/'pioneer_q38_manual_watchdog_host.cpp'),'-o',str(watchdog_object)],check=True)
        extra += [str(watchdog_object)]
    profile_environment={}
    if model_recurrent:profile_environment.update(Q38_TEST_MODEL_ALPHA=str(model_recurrent[0]),Q38_TEST_MODEL_BETA=str(model_recurrent[1]))
    if model_qkv:profile_environment['Q38_TEST_MODEL_QKV']=str(model_qkv)
    if model_gate:profile_environment['Q38_TEST_MODEL_GATE']=str(model_gate)
    if model_output:profile_environment['Q38_TEST_MODEL_OUTPUT']=str(model_output)
    if counter_gather:
        # Authenticate ALL frozen artifacts before classifying any TEST
        # request. nm consumes the SAME already authenticated ELF snapshot,
        # not a second unchecked open of the original image pathname.
        tree=ast.parse((here.parent/('build_qwen38_token_phase_profile_root.py' if token_phase_profile else 'build_qwen38_batched_output_closure_root.py' if output_closure else 'build_qwen38_batched_prefetch_root.py' if batched_prefetch or twin_batch_prefetch else 'build_qwen38_flat_matrix_grammar_root.py' if flat_matrix else 'build_qwen38_empty_receipt_poll_root.py' if empty_receipt else 'build_qwen38_counter_gather_root.py')).read_text())
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
        if recurrent_peer:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_TEST=1']
        if recurrent_matrix_peer:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_MATRIX_TEST=1']
        if output_producers:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_OUTPUT_TEST=1']
        if ffn_producers:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_FFN_TEST=1']
        if ffn_matrix:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_FFN_MATRIX_TEST=1','-DPIONEER_Q38_NATIVE_BFP4_PEER_TEST=1']
        if ffn_up:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_FFN_UP_TEST=1']
        if ffn_activation:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_RECURRENT_FFN_ACTIVATION_TEST=1']
        if recurrent_producer:
            peer_flags += ['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1','-DPIONEER_Q38_TEST_BF16_CONNECTED_PREPARE=1']
        if precise_context:
            peer_flags += ['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1']  # DATA only;never native/kernel roles
        if pushed_context:
            peer_flags += ['-DPIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1']  # distinct HOST DATA,NEVER BUILD/native/kernel flags
        if precise_beta:
            # Observe peers1..119 ONLY through the selected const-ROM TU.
            # In particular the wide family has its own complete new peer
            # image;never compare it against the old narrow-beta arrays.
            # ROOT code comes from the independently authenticated package.
            peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_PRECISE_BETA_TEST=1']
            if matrix_phase:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_MATRIX_PHASE_TEST=1']
            if manual_ttsync:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_MANUAL_TTSYNC_TEST=1']
            if manual_watchdog:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_MANUAL_WATCHDOG_TEST=1']
            if wide_gdn:
                peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_WIDE_GDN_TEST=1','-DPIONEER_Q38_WIDE_GDN_RETURNING_WORKER_TEST=1']
            if twin_batch_prefetch:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_TWIN_BATCH_PREFETCH_TEST=1']
            elif batched_prefetch:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_BATCHED_PREFETCH_TEST=1']
            elif prefetch_worker:peer_flags += ['-DPIONEER_Q38_NATIVE_PEER_PREFETCH_TEST=1']
            rom_object=base/'precise-beta-expected-rom-data.o'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined','-c',str(here/('pioneer_q38_token_manual_ttsync_program_data_bridge.cpp' if manual_ttsync else 'pioneer_q38_token_matrix_phase_program_data_bridge.cpp' if matrix_phase else 'pioneer_q38_token_constant_q4_program_data_bridge.cpp' if constant_q4 else 'pioneer_q38_token_phase_profile_program_data_bridge.cpp' if token_phase_profile else 'pioneer_q38_wide_gdn_program_data_bridge.cpp' if wide_gdn else 'pioneer_q38_batched_output_closure_program_data_bridge.cpp' if output_closure else 'pioneer_q38_twin_batch_prefetch_program_data_bridge.cpp' if twin_batch_prefetch else 'pioneer_q38_batched_prefetch_program_data_bridge.cpp' if batched_prefetch else 'pioneer_q38_prefetch_program_data_bridge.cpp' if prefetch_worker else 'pioneer_q38_precise_beta_program_data_bridge.cpp')),
                            '-o',str(rom_object)],check=True)
            extra += [str(rom_object)] # DATA TU has NO inherited tensor-Work/native/CPU-oracle roles
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', *peer_flags, '-c', str(here/'pioneer_q38_native_peer_test_bridge.cpp'), '-o', str(peer_object)], check=True)
        extra += [str(peer_object)]  # HOST-only TU;never native/kernel-role flags
        if recurrent_matrix_peer:
            bfp_object=base/'native-bfp-peer.o'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                            '-Wno-misleading-indentation','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                            '-DPIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT=1',
                            *(['-DPIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION=1'] if output_matrix else []),
                            '-c',str(here/'pioneer_q38_native_bfp_peer_bridge.cpp'),'-o',str(bfp_object)],check=True)
            extra += [str(bfp_object)] # host tensor/queued-read endpoint;no kernel or Work capability
        if ffn_matrix:
            bfp4_object=base/'native-bfp4-peer.o'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                            '-Wno-misleading-indentation','-Wno-unused-function','-ffp-contract=off','-fsanitize=address,undefined',
                            '-DPIONEER_Q38_NATIVE_BFP4_PEER_TEST=1',
                            '-c',str(here/'pioneer_q38_native_bfp4_peer_bridge.cpp'),'-o',str(bfp4_object)],check=True)
            extra += [str(bfp4_object)]
    if embedding or full_end_contract:
        # RAW byte transport is a separate TEST-only TU,without native-role
        # defines. Never promote its CPU-validation preprocessor role into
        # the native factory/driver candidate or its SDK/Work build.
        if not synthetic_embedding:
            raw_object = base/'original-raw.o'
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                            '-fsanitize=address,undefined', '-c', str(here/'pioneer_q38_whole_original_bytes.cpp'), '-o', str(raw_object)], check=True)
            extra += [str(raw_object)]
        bridge_object = base/'embedding-raw-bridge.o'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', *(['-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1'] if synthetic_embedding else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST=1'] if synthetic_layer else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST=1'] if synthetic_recurrent_matrix else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1'] if gate_cohort else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1'] if qkv_cohort else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1'] if conv_cohort else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST=1'] if gdn_cohort else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST=1'] if output_matrix else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST=1'] if ffn_matrix else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST=1'] if ffn_up else []),
                        *(['-DPIONEER_Q38_SYNTHETIC_FFN_DOWN_MATRIX_TEST=1'] if ffn_down else []),
                        *(['-DPIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST=1'] if model_recurrent else []),
                        *(['-DPIONEER_Q38_MODEL_QKV_OBSERVER_TEST=1'] if model_qkv else []),
                        *(['-DPIONEER_Q38_MODEL_GATE_OBSERVER_TEST=1'] if model_gate else []),
                        *(['-DPIONEER_Q38_MODEL_OUTPUT_OBSERVER_TEST=1'] if model_output else []),
                        '-c', str(here/('pioneer_q38_synthetic_embedding_bridge.cpp' if synthetic_embedding else 'pioneer_q38_embedding_raw_test_bridge.cpp')), '-o', str(bridge_object)], check=True)
        if synthetic_layer:
            fixture_test=base/'synthetic-layer-test'
            subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined',str(here/'pioneer_q38_synthetic_qgw8_layer_test.cpp'),
                            str(bridge_object),'-o',str(fixture_test)],check=True)
            subprocess.run([str(fixture_test),sys.argv[3],str(here/'synthetic_q4_embedding.fixture')],check=True,
                           env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1',**profile_environment})
            for forbidden in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
                              'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
                result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST=1','-D'+forbidden,'-fsyntax-only',
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
                assert result.returncode and 'Synthetic embedding bytes require their isolated host-test implementation' in result.stderr,forbidden
            if model_qkv:
                result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1',
                    '-DPIONEER_Q38_MODEL_QKV_OBSERVER_TEST=1','-fsyntax-only',
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
                assert result.returncode and 'Model QKV bytes require the isolated model recurrent QKV fixture' in result.stderr
            if model_gate:
                result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1',
                    '-DPIONEER_Q38_MODEL_GATE_OBSERVER_TEST=1','-fsyntax-only',
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
                assert result.returncode and 'Model gate bytes require the isolated model QKV fixture' in result.stderr
            if model_output:
                result=subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1',
                    '-DPIONEER_Q38_MODEL_OUTPUT_OBSERVER_TEST=1','-fsyntax-only',
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
                assert result.returncode and 'Model output bytes require the isolated model gate and output fixture' in result.stderr
        extra += [str(bridge_object)]
    if model_recurrent:
        observer=base/'model-recurrent-observer.o'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-fsanitize=address,undefined','-DPIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST=1',
                        '-c',str(here/'pioneer_q38_model_recurrent_observer.cpp'),'-o',str(observer)],check=True)
        extra.append(str(observer))
    if gdn_cohort:
        gdn_observer=base/'synthetic-gdn-observer.o'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-DPIONEER_Q38_SYNTHETIC_GDN_OBSERVER_TEST=1',
                        '-c',str(here/'pioneer_q38_synthetic_gdn_observer.cpp'),'-o',str(gdn_observer)],check=True)
        extra.append(str(gdn_observer))
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *['-D'+k+'='+v for k, v in sorted(defines.items())],
                    '-I'+str(base), '-I'+str(kernel), '-I'+str(here.parent),
                    str(here/('pioneer_q38_hybrid_native_initialization_test.cpp' if hybrid_header else 'pioneer_q38_token_native_initialization_test.cpp')),
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
        if model_recurrent:
            for p,digest in zip(model_recurrent,('92f7287e9626c4dae6ff4c1118d6d2c64f194cf1c239ac3f7b1b60e477ed4fbb','f6615b16825dbe365aad7d01729b2b6cbd19c2f0d51545890d1fe74933bcb753')):
                assert hashlib.sha256(p.read_bytes()).hexdigest()==digest
        if model_qkv:assert hashlib.sha256(model_qkv.read_bytes()).hexdigest()=='6146121bb87b7a684d544b73e06a62f0a6b05513866715bfdbff3dfc840d514a'
        if model_gate:assert hashlib.sha256(model_gate.read_bytes()).hexdigest()=='e3c1189211445020054aeb42ccd89e0d968fb39acd35d0be99ff64755fb1f02b'
        if model_output:assert hashlib.sha256(model_output.read_bytes()).hexdigest()=='8158c4fbaa3569ae7f3453d367b7d988a6d67d0107817c7af524308f1814d886'
