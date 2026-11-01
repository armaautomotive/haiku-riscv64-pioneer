#!/usr/bin/env python3
"""Actual /bin/sh updater in disposable fake Haiku filesystems;NO device IO."""
import hashlib
import pathlib
import shlex
import shutil
import subprocess
import sys
import tempfile
from pioneer_q38_installer_guards_test import assignments

SCRIPT = pathlib.Path(__file__).resolve().parent.parent / 'pioneer_p100a_hot_swap.sh'
TOKEN_REVISIONS = ('token_validation', 'token_hold_recheck', 'token_root_frontend')
TOKEN_MODES = tuple('--'+direction+'-'+revision.replace('_', '-')
                    for revision in TOKEN_REVISIONS for direction in ('apply', 'restore'))
RETAINED_MODES = ('--apply-token-retained-cohort-benchmark', '--restore-token-retained-cohort-benchmark')
COMMIT_MODES = ('--apply-token-commit-boundary-benchmark', '--restore-token-commit-boundary-benchmark')
COHORT_SOURCE_MODES = ('--apply-token-cohort-sources-benchmark', '--restore-token-cohort-sources-benchmark')
PACKET_IDENTITY_MODES = ('--apply-token-packet-identity-benchmark', '--restore-token-packet-identity-benchmark')
TID_BOUNDARY_MODES = ('--apply-token-tid-boundary-benchmark', '--restore-token-tid-boundary-benchmark')
COUNTER_GATHER_MODES = ('--apply-token-counter-gather-benchmark', '--restore-token-counter-gather-benchmark')
EMPTY_RECEIPT_POLL_MODES = ('--apply-token-empty-receipt-poll-benchmark', '--restore-token-empty-receipt-poll-benchmark')
FLAT_MATRIX_MODES = ('--apply-token-flat-matrix-benchmark', '--restore-token-flat-matrix-benchmark')
PREFETCH_MODES = ('--apply-token-prefetch-benchmark', '--restore-token-prefetch-benchmark')
BATCHED_PREFETCH_MODES = ('--apply-token-batched-prefetch-benchmark', '--restore-token-batched-prefetch-benchmark')
BATCHED_PREFETCH_OLD_MODES = PREFETCH_MODES + FLAT_MATRIX_MODES + EMPTY_RECEIPT_POLL_MODES + COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
TWIN_BATCH_PREFETCH_MODES = ('--apply-token-twin-batch-prefetch-benchmark', '--restore-token-twin-batch-prefetch-benchmark')
TWIN_BATCH_PREFETCH_OLD_REVISIONS = (
    'token_batched_prefetch_benchmark', 'token_prefetch_benchmark', 'token_flat_matrix_benchmark',
    'token_empty_receipt_poll_benchmark', 'token_counter_gather_benchmark', 'token_tid_boundary_benchmark',
    'token_packet_identity_benchmark', 'token_cohort_sources_benchmark', 'token_commit_boundary_benchmark',
    'token_retained_cohort_benchmark', 'token_frontier_benchmark', 'token_decode_benchmark',
    'token_job_boot_fold', 'token_precise_beta', 'token_posted_cancel', 'token_fault_readout',
    'token_hidden_readout', *TOKEN_REVISIONS,
)
TWIN_BATCH_PREFETCH_OLD_MODES = tuple('--' + direction + '-' + revision.replace('_', '-')
                                    for revision in TWIN_BATCH_PREFETCH_OLD_REVISIONS
                                    for direction in ('apply', 'restore'))


def check(mode, fault=None, legacy_backup=False, qualified_backup=False, other_token_backup=False):
    text = SCRIPT.read_text()
    values = assignments(text)
    with tempfile.TemporaryDirectory(prefix='pioneer-hot-swap-') as name:
        root = pathlib.Path(name)
        def path(value):
            assert value.startswith('/boot/home/')
            return root / value.lstrip('/')
        def put(key, data):
            target = path(values[key])
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            return target
        payloads = {key: ('test-' + values[key]).encode() for key in ['original', 'hasha', 'hashb', 'hashp', 'hashq', 'hashc', 'hashf', 'hashr', 'hashx', 'hashw', 'hashwp', 'hashws', 'hashwc', 'hashw4', 'hashw4i', 'hashwsp', 'hashwspf', 'hashwspc', 'hashwspi', 'hashwspq', 'hashwsps', 'hashwspa', 'hashwspu', 'hashwspd', 'hashwspo', 'hashwspj', 'hashwspv', 'hashwspg', 'guardsha']}
        payloads['hashcurrentprofile'] = ('test-' + values['hashcurrentprofile']).encode()
        payloads['hash_postscale_delta'] = ('test-' + values['hash_postscale_delta']).encode()
        payloads['hash_gdn_zero_diagnostic'] = ('test-' + values['hash_gdn_zero_diagnostic']).encode()
        payloads['hash_resident_root_diagnostic'] = ('test-' + values['hash_resident_root_diagnostic']).encode()
        payloads['hash_root_cycle_diagnostic'] = ('test-' + values['hash_root_cycle_diagnostic']).encode()
        payloads['hash_block_affine_qualification'] = ('test-' + values['hash_block_affine_qualification']).encode()
        payloads['hash_scale_reuse_qualification'] = ('test-' + values['hash_scale_reuse_qualification']).encode()
        payloads['hash_group32_qualification'] = ('test-' + values['hash_group32_qualification']).encode()
        payloads['hash_native_q8_qualification'] = ('test-' + values['hash_native_q8_qualification']).encode()
        payloads['hash_affine_rawdot_qualification'] = ('test-' + values['hash_affine_rawdot_qualification']).encode()
        payloads['hash_direct_srcb_qualification'] = ('test-' + values['hash_direct_srcb_qualification']).encode()
        payloads['hash_sentinel_poll_qualification'] = ('test-' + values['hash_sentinel_poll_qualification']).encode()
        payloads['hash_graph_csr_diagnostic'] = ('test-' + values['hash_graph_csr_diagnostic']).encode()
        payloads['hash_ffn_code_diagnostic'] = ('test-' + values['hash_ffn_code_diagnostic']).encode()
        payloads['hash_ffn_peer_lifetime_diagnostic'] = ('test-' + values['hash_ffn_peer_lifetime_diagnostic']).encode()
        payloads['hash_ffn_root_lifetime_diagnostic'] = ('test-' + values['hash_ffn_root_lifetime_diagnostic']).encode()
        payloads['hash_ffn_model_init'] = ('test-' + values['hash_ffn_model_init']).encode()
        payloads['hash_ffn_native_graph'] = ('test-' + values['hash_ffn_native_graph']).encode()
        payloads['hash_ffn_host_profile'] = ('test-' + values['hash_ffn_host_profile']).encode()
        payloads['hash_ffn_sealed_proof'] = ('test-' + values['hash_ffn_sealed_proof']).encode()
        payloads['hash_embedding_validation'] = ('test-' + values['hash_embedding_validation']).encode()
        payloads['hash_rms_validation'] = ('test-' + values['hash_rms_validation']).encode()
        payloads['hash_projection_validation'] = ('test-' + values['hash_projection_validation']).encode()
        payloads['hash_decoder_validation'] = ('test-' + values['hash_decoder_validation']).encode()
        payloads['hash_gdn_input_validation'] = ('test-' + values['hash_gdn_input_validation']).encode()
        payloads['hash_gdn_recurrent_validation'] = ('test-' + values['hash_gdn_recurrent_validation']).encode()
        payloads['hash_gdn_rendezvous_validation'] = ('test-' + values['hash_gdn_rendezvous_validation']).encode()
        payloads['hash_conv_silu_precision_validation'] = ('test-' + values['hash_conv_silu_precision_validation']).encode()
        payloads['hash_resident_output_validation'] = ('test-' + values['hash_resident_output_validation']).encode()
        payloads['hash_resident_layer_validation'] = ('test-' + values['hash_resident_layer_validation']).encode()
        payloads['hash_mixed_live_validation'] = ('test-' + values['hash_mixed_live_validation']).encode()
        payloads['hash_recurrent_block_validation'] = ('test-' + values['hash_recurrent_block_validation']).encode()
        payloads['hash_recurrent_deadline_validation'] = ('test-' + values['hash_recurrent_deadline_validation']).encode()
        payloads['hash_recurrent_prefix_validation'] = ('test-' + values['hash_recurrent_prefix_validation']).encode()
        payloads['hash_token_validation'] = ('test-' + values['hash_token_validation']).encode()
        payloads['hash_token_hold_recheck'] = ('test-' + values['hash_token_hold_recheck']).encode()
        payloads['hash_token_root_frontend'] = ('test-' + values['hash_token_root_frontend']).encode()
        payloads['hash_token_hidden_readout'] = ('test-' + values['hash_token_hidden_readout']).encode()
        payloads['hash_token_fault_readout'] = ('test-' + values['hash_token_fault_readout']).encode()
        payloads['hash_token_posted_cancel'] = ('test-' + values['hash_token_posted_cancel']).encode()
        payloads['hash_token_precise_beta'] = ('test-' + values['hash_token_precise_beta']).encode()
        payloads['hash_token_job_boot_fold'] = ('test-' + values['hash_token_job_boot_fold']).encode()
        payloads['hash_token_decode_benchmark'] = ('test-' + values['hash_token_decode_benchmark']).encode()
        payloads['hash_token_frontier_benchmark'] = ('test-' + values['hash_token_frontier_benchmark']).encode()
        payloads['hash_token_retained_cohort_benchmark'] = ('test-' + values['hash_token_retained_cohort_benchmark']).encode()
        payloads['hash_token_commit_boundary_benchmark'] = ('test-' + values['hash_token_commit_boundary_benchmark']).encode()
        payloads['hash_token_cohort_sources_benchmark'] = ('test-' + values['hash_token_cohort_sources_benchmark']).encode()
        payloads['hash_token_packet_identity_benchmark'] = ('test-' + values['hash_token_packet_identity_benchmark']).encode()
        payloads['hash_token_tid_boundary_benchmark'] = ('test-' + values['hash_token_tid_boundary_benchmark']).encode()
        payloads['hash_token_counter_gather_benchmark'] = ('test-' + values['hash_token_counter_gather_benchmark']).encode()
        payloads['hash_token_empty_receipt_poll_benchmark'] = ('test-' + values['hash_token_empty_receipt_poll_benchmark']).encode()
        payloads['hash_token_flat_matrix_benchmark'] = ('test-' + values['hash_token_flat_matrix_benchmark']).encode()
        payloads['hash_token_prefetch_benchmark'] = ('test-' + values['hash_token_prefetch_benchmark']).encode()
        payloads['hash_token_batched_prefetch_benchmark'] = ('test-' + values['hash_token_batched_prefetch_benchmark']).encode()
        payloads['hash_token_twin_batch_prefetch_benchmark'] = ('test-' + values['hash_token_twin_batch_prefetch_benchmark']).encode()
        payloads['hash_host_phase_diagnostic'] = ('test-' + values['hash_host_phase_diagnostic']).encode()
        payloads['hash_batch4_fetch_qualification'] = ('test-' + values['hash_batch4_fetch_qualification']).encode()
        payloads['hash_batch_raw_qualification'] = ('test-' + values['hash_batch_raw_qualification']).encode()
        payloads['hash_frontend_qualification'] = ('test-' + values['hash_frontend_qualification']).encode()
        payloads['hash_raw_unpack_cache_qualification'] = ('test-' + values['hash_raw_unpack_cache_qualification']).encode()
        payloads['hash_tile_prefetch_qualification'] = ('test-' + values['hash_tile_prefetch_qualification']).encode()
        payloads['hash_command_reuse_qualification'] = ('test-' + values['hash_command_reuse_qualification']).encode()
        payloads['hash_peer_phase_qualification'] = ('test-' + values['hash_peer_phase_qualification']).encode()
        payloads['hash_q4_nibble_qualification'] = ('test-' + values['hash_q4_nibble_qualification']).encode()
        payloads['hash_q4_direct_qualification'] = ('test-' + values['hash_q4_direct_qualification']).encode()
        payloads['hash_local_q4_qualification'] = ('test-' + values['hash_local_q4_qualification']).encode()
        for key in ['hash_sparse_profile', 'hash_sparse_export', 'hash_normal_clock', 'hash_sum_cache', 'hash_zero_minimum', 'hash_native_operand', 'hash_compact_output', 'hash_compact_profile', 'hash_deferred_affine', 'hash_ordered_extents', 'hash_inline_config']:
            payloads[key] = ('test-' + values[key]).encode()
        routes = {
            '--bootstrap': ('original', 'hasha', 'taga'),
            '--apply-a': ('hashb', 'hasha', 'tagb'), '--apply-b': ('hasha', 'hashb', 'taga'),
            '--apply-profile': ('hasha', 'hashp', 'taga'), '--restore-profile': ('hashp', 'hasha', 'tagp'),
            '--apply-batch': ('hashp', 'hashq', 'tagp'), '--restore-batch': ('hashq', 'hasha', 'tagq'),
            '--apply-compact': ('hashq', 'hashc', 'tagq'), '--restore-compact': ('hashc', 'hasha', 'tagc'),
            '--apply-fast': ('hashc', 'hashf', 'tagc'), '--restore-fast': ('hashf', 'hasha', 'tagf'),
            '--apply-reuse': ('hashf', 'hashr', 'tagf'), '--restore-reuse': ('hashr', 'hashf', 'tagr'),
            '--apply-cross-reuse': ('hashr', 'hashx', 'tagr'), '--restore-cross-reuse': ('hashx', 'hashr', 'tagx'),
            '--apply-work': ('hashx', 'hashw', 'tagx'), '--restore-work': ('hashw', 'hashx', 'tagw'),
            '--apply-work-profile': ('hashw', 'hashwp', 'tagw'), '--restore-work-profile': ('hashwp', 'hashw', 'tagwp'),
            '--apply-work-posted': ('hashw', 'hashws', 'tagw'), '--restore-work-posted': ('hashws', 'hashw', 'tagws'),
            '--apply-work-cohort': ('hashws', 'hashwc', 'tagws'), '--restore-work-cohort': ('hashwc', 'hashws', 'tagwc'),
            '--apply-work-q4': ('hashwc', 'hashw4', 'tagwc'), '--restore-work-q4': ('hashw4', 'hashwc', 'tagw4'),
            '--apply-work-q4-inline': ('hashw4', 'hashw4i', 'tagw4'), '--restore-work-q4-inline': ('hashw4i', 'hashw4', 'tagw4i'),
            '--apply-work-span': ('hashw4i', 'hashwsp', 'tagw4i'), '--restore-work-span': ('hashwsp', 'hashw4i', 'tagwsp'),
            '--apply-work-fast-span': ('hashwsp', 'hashwspf', 'tagwsp'), '--restore-work-fast-span': ('hashwspf', 'hashwsp', 'tagwspf'),
            '--apply-work-span-code-reuse': ('hashwspf', 'hashwspc', 'tagwspf'), '--restore-work-span-code-reuse': ('hashwspc', 'hashwspf', 'tagwspc'),
            '--apply-work-int-sfpu': ('hashwspc', 'hashwspi', 'tagwspc'), '--restore-work-int-sfpu': ('hashwspi', 'hashwspc', 'tagwspi'),
            '--apply-work-pipelined': ('hashwspi', 'hashwspq', 'tagwspi'), '--restore-work-pipelined': ('hashwspq', 'hashwspi', 'tagwspq'),
            '--apply-work-static-vector': ('hashwspq', 'hashwsps', 'tagwspq'), '--restore-work-static-vector': ('hashwsps', 'hashwspq', 'tagwsps'),
            '--apply-work-cached-affine': ('hashwsps', 'hashwspa', 'tagwsps'), '--restore-work-cached-affine': ('hashwspa', 'hashwsps', 'tagwspa'),
            '--apply-work-replay-operand': ('hashwspa', 'hashwspu', 'tagwspa'), '--restore-work-replay-operand': ('hashwspu', 'hashwspa', 'tagwspu'),
            '--apply-work-dot': ('hashwspu', 'hashwspd', 'tagwspu'), '--restore-work-dot': ('hashwspd', 'hashwspu', 'tagwspd'),
            '--apply-work-output-dma': ('hashwspd', 'hashwspo', 'tagwspd'), '--restore-work-output-dma': ('hashwspo', 'hashwspd', 'tagwspo'),
            '--apply-work-queued-entry': ('hashwspo', 'hashwspj', 'tagwspo'), '--restore-work-queued-entry': ('hashwspj', 'hashwspo', 'tagwspj'),
            '--apply-local-delta': ('hashwspj', 'hashwspv', 'tagwspj'), '--restore-local-delta': ('hashwspv', 'hashwspj', 'tagwspv'),
            '--apply-local-delta-terminal': ('hashwspv', 'hashwspg', 'tagwspv'), '--restore-local-delta-terminal': ('hashwspg', 'hashwspv', 'tagwspg'),
            '--apply-current-profile': ('hashwspg', 'hashcurrentprofile', 'tagwspg'),
            '--restore-current-profile': ('hashcurrentprofile', 'hashwspg', 'tagcurrentprofile'),
            '--apply-sparse-profile': ('hashcurrentprofile', 'hash_sparse_profile', 'tagcurrentprofile'),
            '--restore-sparse-profile': ('hash_sparse_profile', 'hashcurrentprofile', 'tag_sparse_profile'),
            '--apply-sparse-export': ('hash_sparse_profile', 'hash_sparse_export', 'tag_sparse_profile'),
            '--restore-sparse-export': ('hash_sparse_export', 'hash_sparse_profile', 'tag_sparse_export'),
            '--apply-normal-clock': ('hash_sparse_export', 'hash_normal_clock', 'tag_sparse_export'),
            '--restore-normal-clock': ('hash_normal_clock', 'hash_sparse_export', 'tag_normal_clock'),
            '--apply-input-sum-cache': ('hash_normal_clock', 'hash_sum_cache', 'tag_normal_clock'),
            '--restore-input-sum-cache': ('hash_sum_cache', 'hash_normal_clock', 'tag_sum_cache'),
            '--apply-zero-minimum': ('hash_sum_cache', 'hash_zero_minimum', 'tag_sum_cache'),
            '--restore-zero-minimum': ('hash_zero_minimum', 'hash_sum_cache', 'tag_zero_minimum'),
            '--apply-native-operand': ('hash_zero_minimum', 'hash_native_operand', 'tag_zero_minimum'),
            '--restore-native-operand': ('hash_native_operand', 'hash_zero_minimum', 'tag_native_operand'),
            '--apply-compact-output': ('hash_native_operand', 'hash_compact_output', 'tag_native_operand'),
            '--restore-compact-output': ('hash_compact_output', 'hash_native_operand', 'tag_compact_output'),
            '--apply-compact-profile': ('hash_compact_output', 'hash_compact_profile', 'tag_compact_output'),
            '--restore-compact-profile': ('hash_compact_profile', 'hash_compact_output', 'tag_compact_profile'),
            '--apply-deferred-affine': ('hash_compact_output', 'hash_deferred_affine', 'tag_compact_output'),
            '--restore-deferred-affine': ('hash_deferred_affine', 'hash_compact_output', 'tag_deferred_affine'),
            '--apply-ordered-extents': ('hash_compact_output', 'hash_ordered_extents', 'tag_compact_output'),
            '--restore-ordered-extents': ('hash_ordered_extents', 'hash_compact_output', 'tag_ordered_extents'),
            '--apply-inline-config': ('hash_deferred_affine', 'hash_inline_config', 'tag_deferred_affine'),
            '--restore-inline-config': ('hash_inline_config', 'hash_deferred_affine', 'tag_inline_config'),
            '--apply-postscale-delta': ('hash_inline_config', 'hash_postscale_delta', 'tag_inline_config'),
            '--restore-postscale-delta': ('hash_postscale_delta', 'hash_inline_config', 'tag_postscale_delta'),
            '--apply-gdn-zero-diagnostic': ('hash_postscale_delta', 'hash_gdn_zero_diagnostic', 'tag_postscale_delta'),
            '--restore-gdn-zero-diagnostic': ('hash_gdn_zero_diagnostic', 'hash_postscale_delta', 'tag_gdn_zero_diagnostic'),
            '--apply-resident-root-diagnostic': ('hash_gdn_zero_diagnostic', 'hash_resident_root_diagnostic', 'tag_gdn_zero_diagnostic'),
            '--restore-resident-root-diagnostic': ('hash_resident_root_diagnostic', 'hash_gdn_zero_diagnostic', 'tag_resident_root_diagnostic'),
            '--apply-root-cycle-diagnostic': ('hash_resident_root_diagnostic', 'hash_root_cycle_diagnostic', 'tag_resident_root_diagnostic'),
            '--restore-root-cycle-diagnostic': ('hash_root_cycle_diagnostic', 'hash_resident_root_diagnostic', 'tag_root_cycle_diagnostic'),
            '--apply-block-affine-qualification': ('hash_root_cycle_diagnostic', 'hash_block_affine_qualification', 'tag_root_cycle_diagnostic'),
            '--restore-block-affine-qualification': ('hash_block_affine_qualification', 'hash_root_cycle_diagnostic', 'tag_block_affine_qualification'),
            '--apply-scale-reuse-qualification': ('hash_block_affine_qualification', 'hash_scale_reuse_qualification', 'tag_block_affine_qualification'),
            '--restore-scale-reuse-qualification': ('hash_scale_reuse_qualification', 'hash_block_affine_qualification', 'tag_scale_reuse_qualification'),
            '--apply-group32-qualification': ('hash_scale_reuse_qualification', 'hash_group32_qualification', 'tag_scale_reuse_qualification'),
            '--restore-group32-qualification': ('hash_group32_qualification', 'hash_scale_reuse_qualification', 'tag_group32_qualification'),
            '--apply-native-q8-qualification': ('hash_scale_reuse_qualification', 'hash_native_q8_qualification', 'tag_scale_reuse_qualification'),
            '--restore-native-q8-qualification': ('hash_native_q8_qualification', 'hash_scale_reuse_qualification', 'tag_native_q8_qualification'),
            '--apply-affine-rawdot-qualification': ('hash_scale_reuse_qualification', 'hash_affine_rawdot_qualification', 'tag_scale_reuse_qualification'),
            '--restore-affine-rawdot-qualification': ('hash_affine_rawdot_qualification', 'hash_scale_reuse_qualification', 'tag_affine_rawdot_qualification'),
            '--apply-direct-srcb-qualification': ('hash_scale_reuse_qualification', 'hash_direct_srcb_qualification', 'tag_scale_reuse_qualification'),
            '--restore-direct-srcb-qualification': ('hash_direct_srcb_qualification', 'hash_scale_reuse_qualification', 'tag_direct_srcb_qualification'),
            '--apply-sentinel-poll-qualification': ('hash_direct_srcb_qualification', 'hash_sentinel_poll_qualification', 'tag_direct_srcb_qualification'),
            '--restore-sentinel-poll-qualification': ('hash_sentinel_poll_qualification', 'hash_direct_srcb_qualification', 'tag_sentinel_poll_qualification'),
            '--apply-graph-csr-diagnostic': ('hash_scale_reuse_qualification', 'hash_graph_csr_diagnostic', 'tag_scale_reuse_qualification'),
            '--restore-graph-csr-diagnostic': ('hash_graph_csr_diagnostic', 'hash_scale_reuse_qualification', 'tag_graph_csr_diagnostic'),
            '--apply-ffn-code-diagnostic': ('hash_scale_reuse_qualification', 'hash_ffn_code_diagnostic', 'tag_scale_reuse_qualification'),
            '--restore-ffn-code-diagnostic': ('hash_ffn_code_diagnostic', 'hash_scale_reuse_qualification', 'tag_ffn_code_diagnostic'),
            '--apply-ffn-peer-lifetime-diagnostic': ('hash_scale_reuse_qualification', 'hash_ffn_peer_lifetime_diagnostic', 'tag_scale_reuse_qualification'),
            '--restore-ffn-peer-lifetime-diagnostic': ('hash_ffn_peer_lifetime_diagnostic', 'hash_scale_reuse_qualification', 'tag_ffn_peer_lifetime_diagnostic'),
            '--apply-ffn-root-lifetime-diagnostic': ('hash_scale_reuse_qualification', 'hash_ffn_root_lifetime_diagnostic', 'tag_scale_reuse_qualification'),
            '--restore-ffn-root-lifetime-diagnostic': ('hash_ffn_root_lifetime_diagnostic', 'hash_scale_reuse_qualification', 'tag_ffn_root_lifetime_diagnostic'),
            '--apply-ffn-model-init': ('hash_scale_reuse_qualification', 'hash_ffn_model_init', 'tag_scale_reuse_qualification'),
            '--restore-ffn-model-init': ('hash_ffn_model_init', 'hash_scale_reuse_qualification', 'tag_ffn_model_init'),
            '--apply-ffn-native-graph': ('hash_scale_reuse_qualification', 'hash_ffn_native_graph', 'tag_scale_reuse_qualification'),
            '--restore-ffn-native-graph': ('hash_ffn_native_graph', 'hash_scale_reuse_qualification', 'tag_ffn_native_graph'),
            '--apply-ffn-host-profile': ('hash_scale_reuse_qualification', 'hash_ffn_host_profile', 'tag_scale_reuse_qualification'),
            '--restore-ffn-host-profile': ('hash_ffn_host_profile', 'hash_scale_reuse_qualification', 'tag_ffn_host_profile'),
            '--apply-ffn-sealed-proof': ('hash_scale_reuse_qualification', 'hash_ffn_sealed_proof', 'tag_scale_reuse_qualification'),
            '--restore-ffn-sealed-proof': ('hash_ffn_sealed_proof', 'hash_scale_reuse_qualification', 'tag_ffn_sealed_proof'),
            '--apply-embedding-validation': ('hash_scale_reuse_qualification', 'hash_embedding_validation', 'tag_scale_reuse_qualification'),
            '--restore-embedding-validation': ('hash_embedding_validation', 'hash_scale_reuse_qualification', 'tag_embedding_validation'),
            '--apply-rms-validation': ('hash_scale_reuse_qualification', 'hash_rms_validation', 'tag_scale_reuse_qualification'),
            '--restore-rms-validation': ('hash_rms_validation', 'hash_scale_reuse_qualification', 'tag_rms_validation'),
            '--apply-projection-validation': ('hash_scale_reuse_qualification', 'hash_projection_validation', 'tag_scale_reuse_qualification'),
            '--restore-projection-validation': ('hash_projection_validation', 'hash_scale_reuse_qualification', 'tag_projection_validation'),
            '--apply-decoder-validation': ('hash_scale_reuse_qualification', 'hash_decoder_validation', 'tag_scale_reuse_qualification'),
            '--restore-decoder-validation': ('hash_decoder_validation', 'hash_scale_reuse_qualification', 'tag_decoder_validation'),
            '--apply-gdn-input-validation': ('hash_scale_reuse_qualification', 'hash_gdn_input_validation', 'tag_scale_reuse_qualification'),
            '--restore-gdn-input-validation': ('hash_gdn_input_validation', 'hash_scale_reuse_qualification', 'tag_gdn_input_validation'),
            '--apply-gdn-recurrent-validation': ('hash_scale_reuse_qualification', 'hash_gdn_recurrent_validation', 'tag_scale_reuse_qualification'),
            '--apply-gdn-rendezvous-validation': ('hash_scale_reuse_qualification', 'hash_gdn_rendezvous_validation', 'tag_scale_reuse_qualification'),
            '--restore-gdn-recurrent-validation': ('hash_gdn_recurrent_validation', 'hash_scale_reuse_qualification', 'tag_gdn_recurrent_validation'),
            '--restore-gdn-rendezvous-validation': ('hash_gdn_rendezvous_validation', 'hash_scale_reuse_qualification', 'tag_gdn_rendezvous_validation'),
            '--apply-conv-silu-precision-validation': ('hash_scale_reuse_qualification', 'hash_conv_silu_precision_validation', 'tag_scale_reuse_qualification'),
            '--restore-conv-silu-precision-validation': ('hash_conv_silu_precision_validation', 'hash_scale_reuse_qualification', 'tag_conv_silu_precision_validation'),
            '--apply-resident-output-validation': ('hash_scale_reuse_qualification', 'hash_resident_output_validation', 'tag_scale_reuse_qualification'),
            '--restore-resident-output-validation': ('hash_resident_output_validation', 'hash_scale_reuse_qualification', 'tag_resident_output_validation'),
            '--apply-resident-layer-validation': ('hash_scale_reuse_qualification', 'hash_resident_layer_validation', 'tag_scale_reuse_qualification'),
            '--restore-resident-layer-validation': ('hash_resident_layer_validation', 'hash_scale_reuse_qualification', 'tag_resident_layer_validation'),
            '--apply-mixed-live-validation': ('hash_scale_reuse_qualification', 'hash_mixed_live_validation', 'tag_scale_reuse_qualification'),
            '--restore-mixed-live-validation': ('hash_mixed_live_validation', 'hash_scale_reuse_qualification', 'tag_mixed_live_validation'),
            '--apply-recurrent-block-validation': ('hash_scale_reuse_qualification', 'hash_recurrent_block_validation', 'tag_scale_reuse_qualification'),
            '--restore-recurrent-block-validation': ('hash_recurrent_block_validation', 'hash_scale_reuse_qualification', 'tag_recurrent_block_validation'),
            '--apply-recurrent-deadline-validation': ('hash_scale_reuse_qualification', 'hash_recurrent_deadline_validation', 'tag_scale_reuse_qualification'),
            '--restore-recurrent-deadline-validation': ('hash_recurrent_deadline_validation', 'hash_scale_reuse_qualification', 'tag_recurrent_deadline_validation'),
            '--apply-recurrent-prefix-validation': ('hash_scale_reuse_qualification', 'hash_recurrent_prefix_validation', 'tag_scale_reuse_qualification'),
            '--restore-recurrent-prefix-validation': ('hash_recurrent_prefix_validation', 'hash_scale_reuse_qualification', 'tag_recurrent_prefix_validation'),
            '--apply-token-validation': ('hash_scale_reuse_qualification', 'hash_token_validation', 'tag_scale_reuse_qualification'),
            '--restore-token-validation': ('hash_token_validation', 'hash_scale_reuse_qualification', 'tag_token_validation'),
            '--apply-token-hold-recheck': ('hash_scale_reuse_qualification', 'hash_token_hold_recheck', 'tag_scale_reuse_qualification'),
            '--restore-token-hold-recheck': ('hash_token_hold_recheck', 'hash_scale_reuse_qualification', 'tag_token_hold_recheck'),
            '--apply-token-root-frontend': ('hash_scale_reuse_qualification', 'hash_token_root_frontend', 'tag_scale_reuse_qualification'),
            '--restore-token-root-frontend': ('hash_token_root_frontend', 'hash_scale_reuse_qualification', 'tag_token_root_frontend'),
            '--apply-token-hidden-readout': ('hash_scale_reuse_qualification', 'hash_token_hidden_readout', 'tag_scale_reuse_qualification'),
            '--restore-token-hidden-readout': ('hash_token_hidden_readout', 'hash_scale_reuse_qualification', 'tag_token_hidden_readout'),
            '--apply-token-fault-readout': ('hash_scale_reuse_qualification', 'hash_token_fault_readout', 'tag_scale_reuse_qualification'),
            '--restore-token-fault-readout': ('hash_token_fault_readout', 'hash_scale_reuse_qualification', 'tag_token_fault_readout'),
            '--apply-token-posted-cancel': ('hash_scale_reuse_qualification', 'hash_token_posted_cancel', 'tag_scale_reuse_qualification'),
            '--restore-token-posted-cancel': ('hash_token_posted_cancel', 'hash_scale_reuse_qualification', 'tag_token_posted_cancel'),
            '--apply-token-precise-beta': ('hash_scale_reuse_qualification', 'hash_token_precise_beta', 'tag_scale_reuse_qualification'),
            '--restore-token-precise-beta': ('hash_token_precise_beta', 'hash_scale_reuse_qualification', 'tag_token_precise_beta'),
            '--apply-token-job-boot-fold': ('hash_scale_reuse_qualification', 'hash_token_job_boot_fold', 'tag_scale_reuse_qualification'),
            '--restore-token-job-boot-fold': ('hash_token_job_boot_fold', 'hash_scale_reuse_qualification', 'tag_token_job_boot_fold'),
            '--apply-token-decode-benchmark': ('hash_scale_reuse_qualification', 'hash_token_decode_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-decode-benchmark': ('hash_token_decode_benchmark', 'hash_scale_reuse_qualification', 'tag_token_decode_benchmark'),
            '--apply-token-frontier-benchmark': ('hash_scale_reuse_qualification', 'hash_token_frontier_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-frontier-benchmark': ('hash_token_frontier_benchmark', 'hash_scale_reuse_qualification', 'tag_token_frontier_benchmark'),
            '--apply-token-retained-cohort-benchmark': ('hash_scale_reuse_qualification', 'hash_token_retained_cohort_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-retained-cohort-benchmark': ('hash_token_retained_cohort_benchmark', 'hash_scale_reuse_qualification', 'tag_token_retained_cohort_benchmark'),
            '--apply-token-commit-boundary-benchmark': ('hash_scale_reuse_qualification', 'hash_token_commit_boundary_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-commit-boundary-benchmark': ('hash_token_commit_boundary_benchmark', 'hash_scale_reuse_qualification', 'tag_token_commit_boundary_benchmark'),
            '--apply-token-cohort-sources-benchmark': ('hash_scale_reuse_qualification', 'hash_token_cohort_sources_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-cohort-sources-benchmark': ('hash_token_cohort_sources_benchmark', 'hash_scale_reuse_qualification', 'tag_token_cohort_sources_benchmark'),
            '--apply-token-packet-identity-benchmark': ('hash_scale_reuse_qualification', 'hash_token_packet_identity_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-packet-identity-benchmark': ('hash_token_packet_identity_benchmark', 'hash_scale_reuse_qualification', 'tag_token_packet_identity_benchmark'),
            '--apply-token-tid-boundary-benchmark': ('hash_scale_reuse_qualification', 'hash_token_tid_boundary_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-tid-boundary-benchmark': ('hash_token_tid_boundary_benchmark', 'hash_scale_reuse_qualification', 'tag_token_tid_boundary_benchmark'),
            '--apply-token-counter-gather-benchmark': ('hash_scale_reuse_qualification', 'hash_token_counter_gather_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-counter-gather-benchmark': ('hash_token_counter_gather_benchmark', 'hash_scale_reuse_qualification', 'tag_token_counter_gather_benchmark'),
            '--apply-token-empty-receipt-poll-benchmark': ('hash_scale_reuse_qualification', 'hash_token_empty_receipt_poll_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-empty-receipt-poll-benchmark': ('hash_token_empty_receipt_poll_benchmark', 'hash_scale_reuse_qualification', 'tag_token_empty_receipt_poll_benchmark'),
            '--apply-token-flat-matrix-benchmark': ('hash_scale_reuse_qualification', 'hash_token_flat_matrix_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-flat-matrix-benchmark': ('hash_token_flat_matrix_benchmark', 'hash_scale_reuse_qualification', 'tag_token_flat_matrix_benchmark'),
            '--apply-token-prefetch-benchmark': ('hash_scale_reuse_qualification', 'hash_token_prefetch_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-prefetch-benchmark': ('hash_token_prefetch_benchmark', 'hash_scale_reuse_qualification', 'tag_token_prefetch_benchmark'),
            '--apply-token-batched-prefetch-benchmark': ('hash_scale_reuse_qualification', 'hash_token_batched_prefetch_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-batched-prefetch-benchmark': ('hash_token_batched_prefetch_benchmark', 'hash_scale_reuse_qualification', 'tag_token_batched_prefetch_benchmark'),
            '--apply-token-twin-batch-prefetch-benchmark': ('hash_scale_reuse_qualification', 'hash_token_twin_batch_prefetch_benchmark', 'tag_scale_reuse_qualification'),
            '--restore-token-twin-batch-prefetch-benchmark': ('hash_token_twin_batch_prefetch_benchmark', 'hash_scale_reuse_qualification', 'tag_token_twin_batch_prefetch_benchmark'),
            '--apply-host-phase-diagnostic': ('hash_sentinel_poll_qualification', 'hash_host_phase_diagnostic', 'tag_sentinel_poll_qualification'),
            '--restore-host-phase-diagnostic': ('hash_host_phase_diagnostic', 'hash_sentinel_poll_qualification', 'tag_host_phase_diagnostic'),
            '--apply-batch4-fetch-qualification': ('hash_sentinel_poll_qualification', 'hash_batch4_fetch_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-batch4-fetch-qualification': ('hash_batch4_fetch_qualification', 'hash_sentinel_poll_qualification', 'tag_batch4_fetch_qualification'),
            '--apply-batch-raw-qualification': ('hash_sentinel_poll_qualification', 'hash_batch_raw_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-batch-raw-qualification': ('hash_batch_raw_qualification', 'hash_sentinel_poll_qualification', 'tag_batch_raw_qualification'),
            '--apply-frontend-qualification': ('hash_sentinel_poll_qualification', 'hash_frontend_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-frontend-qualification': ('hash_frontend_qualification', 'hash_sentinel_poll_qualification', 'tag_frontend_qualification'),
            '--apply-raw-unpack-cache-qualification': ('hash_sentinel_poll_qualification', 'hash_raw_unpack_cache_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-raw-unpack-cache-qualification': ('hash_raw_unpack_cache_qualification', 'hash_sentinel_poll_qualification', 'tag_raw_unpack_cache_qualification'),
            '--apply-tile-prefetch-qualification': ('hash_sentinel_poll_qualification', 'hash_tile_prefetch_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-tile-prefetch-qualification': ('hash_tile_prefetch_qualification', 'hash_sentinel_poll_qualification', 'tag_tile_prefetch_qualification'),
            '--apply-command-reuse-qualification': ('hash_sentinel_poll_qualification', 'hash_command_reuse_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-command-reuse-qualification': ('hash_command_reuse_qualification', 'hash_sentinel_poll_qualification', 'tag_command_reuse_qualification'),
            '--apply-peer-phase-qualification': ('hash_sentinel_poll_qualification', 'hash_peer_phase_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-peer-phase-qualification': ('hash_peer_phase_qualification', 'hash_sentinel_poll_qualification', 'tag_peer_phase_qualification'),
            '--apply-q4-nibble-qualification': ('hash_sentinel_poll_qualification', 'hash_q4_nibble_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-q4-nibble-qualification': ('hash_q4_nibble_qualification', 'hash_sentinel_poll_qualification', 'tag_q4_nibble_qualification'),
            '--apply-q4-direct-qualification': ('hash_sentinel_poll_qualification', 'hash_q4_direct_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-q4-direct-qualification': ('hash_q4_direct_qualification', 'hash_sentinel_poll_qualification', 'tag_q4_direct_qualification'),
            '--apply-local-q4-qualification': ('hash_sentinel_poll_qualification', 'hash_local_q4_qualification', 'tag_sentinel_poll_qualification'),
            '--restore-local-q4-qualification': ('hash_local_q4_qualification', 'hash_sentinel_poll_qualification', 'tag_local_q4_qualification'),
        }
        starting, ending, oldtagkey = routes[mode]
        installed = put('installed', payloads[starting])
        put('candidatea', payloads['hasha'])
        put('candidateb', payloads['hashb'])
        put('candidatep', payloads['hashp'])
        put('candidateq', payloads['hashq'])
        put('candidatec', payloads['hashc'])
        put('candidatef', payloads['hashf'])
        put('candidater', payloads['hashr'])
        put('candidatex', payloads['hashx'])
        put('candidatew', payloads['hashw'])
        put('candidatewp', payloads['hashwp'])
        put('candidatews', payloads['hashws'])
        put('candidatewc', payloads['hashwc'])
        put('candidatew4', payloads['hashw4'])
        put('candidatew4i', payloads['hashw4i'])
        put('candidatewsp', payloads['hashwsp'])
        put('candidatewspf', payloads['hashwspf'])
        put('candidatewspc', payloads['hashwspc'])
        put('candidatewspi', payloads['hashwspi'])
        put('candidatewspq', payloads['hashwspq'])
        put('candidatewsps', payloads['hashwsps'])
        put('candidatewspa', payloads['hashwspa'])
        put('candidatewspu', payloads['hashwspu'])
        put('candidatewspd', payloads['hashwspd'])
        put('candidatewspo', payloads['hashwspo'])
        put('candidatewspj', payloads['hashwspj'])
        put('candidatewspv', payloads['hashwspv'])
        put('candidatewspg', payloads['hashwspg'])
        put('candidatecurrentprofile', payloads['hashcurrentprofile'])
        put('candidate_sparse_profile', payloads['hash_sparse_profile'])
        put('candidate_sparse_export', payloads['hash_sparse_export'])
        put('candidate_normal_clock', payloads['hash_normal_clock'])
        put('candidate_sum_cache', payloads['hash_sum_cache'])
        put('candidate_zero_minimum', payloads['hash_zero_minimum'])
        put('candidate_native_operand', payloads['hash_native_operand'])
        put('candidate_compact_output', payloads['hash_compact_output'])
        put('candidate_compact_profile', payloads['hash_compact_profile'])
        put('candidate_deferred_affine', payloads['hash_deferred_affine'])
        put('candidate_ordered_extents', payloads['hash_ordered_extents'])
        put('candidate_inline_config', payloads['hash_inline_config'])
        put('candidate_postscale_delta', payloads['hash_postscale_delta'])
        put('candidate_gdn_zero_diagnostic', payloads['hash_gdn_zero_diagnostic'])
        put('candidate_resident_root_diagnostic', payloads['hash_resident_root_diagnostic'])
        put('candidate_root_cycle_diagnostic', payloads['hash_root_cycle_diagnostic'])
        put('candidate_block_affine_qualification', payloads['hash_block_affine_qualification'])
        put('candidate_scale_reuse_qualification', payloads['hash_scale_reuse_qualification'])
        put('candidate_group32_qualification', payloads['hash_group32_qualification'])
        put('candidate_native_q8_qualification', payloads['hash_native_q8_qualification'])
        put('candidate_affine_rawdot_qualification', payloads['hash_affine_rawdot_qualification'])
        put('candidate_direct_srcb_qualification', payloads['hash_direct_srcb_qualification'])
        put('candidate_sentinel_poll_qualification', payloads['hash_sentinel_poll_qualification'])
        put('candidate_graph_csr_diagnostic', payloads['hash_graph_csr_diagnostic'])
        put('candidate_ffn_code_diagnostic', payloads['hash_ffn_code_diagnostic'])
        put('candidate_ffn_peer_lifetime_diagnostic', payloads['hash_ffn_peer_lifetime_diagnostic'])
        put('candidate_ffn_root_lifetime_diagnostic', payloads['hash_ffn_root_lifetime_diagnostic'])
        put('candidate_ffn_model_init', payloads['hash_ffn_model_init'])
        put('candidate_ffn_native_graph', payloads['hash_ffn_native_graph'])
        put('candidate_ffn_host_profile', payloads['hash_ffn_host_profile'])
        put('candidate_ffn_sealed_proof', payloads['hash_ffn_sealed_proof'])
        put('candidate_embedding_validation', payloads['hash_embedding_validation'])
        put('candidate_rms_validation', payloads['hash_rms_validation'])
        put('candidate_projection_validation', payloads['hash_projection_validation'])
        put('candidate_decoder_validation', payloads['hash_decoder_validation'])
        put('candidate_gdn_input_validation', payloads['hash_gdn_input_validation'])
        put('candidate_gdn_recurrent_validation', payloads['hash_gdn_recurrent_validation'])
        put('candidate_gdn_rendezvous_validation', payloads['hash_gdn_rendezvous_validation'])
        put('candidate_conv_silu_precision_validation', payloads['hash_conv_silu_precision_validation'])
        put('candidate_resident_output_validation', payloads['hash_resident_output_validation'])
        put('candidate_resident_layer_validation', payloads['hash_resident_layer_validation'])
        put('candidate_mixed_live_validation', payloads['hash_mixed_live_validation'])
        put('candidate_recurrent_block_validation', payloads['hash_recurrent_block_validation'])
        put('candidate_recurrent_deadline_validation', payloads['hash_recurrent_deadline_validation'])
        put('candidate_recurrent_prefix_validation', payloads['hash_recurrent_prefix_validation'])
        put('candidate_token_validation', payloads['hash_token_validation'])
        put('candidate_token_hold_recheck', payloads['hash_token_hold_recheck'])
        put('candidate_token_root_frontend', payloads['hash_token_root_frontend'])
        put('candidate_token_hidden_readout', payloads['hash_token_hidden_readout'])
        put('candidate_token_fault_readout', payloads['hash_token_fault_readout'])
        put('candidate_token_posted_cancel', payloads['hash_token_posted_cancel'])
        put('candidate_token_precise_beta', payloads['hash_token_precise_beta'])
        put('candidate_token_job_boot_fold', payloads['hash_token_job_boot_fold'])
        put('candidate_token_decode_benchmark', payloads['hash_token_decode_benchmark'])
        put('candidate_token_frontier_benchmark', payloads['hash_token_frontier_benchmark'])
        put('candidate_token_retained_cohort_benchmark', payloads['hash_token_retained_cohort_benchmark'])
        put('candidate_token_commit_boundary_benchmark', payloads['hash_token_commit_boundary_benchmark'])
        put('candidate_token_cohort_sources_benchmark', payloads['hash_token_cohort_sources_benchmark'])
        put('candidate_token_packet_identity_benchmark', payloads['hash_token_packet_identity_benchmark'])
        put('candidate_token_tid_boundary_benchmark', payloads['hash_token_tid_boundary_benchmark'])
        put('candidate_token_counter_gather_benchmark', payloads['hash_token_counter_gather_benchmark'])
        put('candidate_token_empty_receipt_poll_benchmark', payloads['hash_token_empty_receipt_poll_benchmark'])
        put('candidate_token_flat_matrix_benchmark', payloads['hash_token_flat_matrix_benchmark'])
        put('candidate_token_prefetch_benchmark', payloads['hash_token_prefetch_benchmark'])
        put('candidate_token_batched_prefetch_benchmark', payloads['hash_token_batched_prefetch_benchmark'])
        put('candidate_token_twin_batch_prefetch_benchmark', payloads['hash_token_twin_batch_prefetch_benchmark'])
        put('candidate_host_phase_diagnostic', payloads['hash_host_phase_diagnostic'])
        put('candidate_batch4_fetch_qualification', payloads['hash_batch4_fetch_qualification'])
        put('candidate_batch_raw_qualification', payloads['hash_batch_raw_qualification'])
        put('candidate_frontend_qualification', payloads['hash_frontend_qualification'])
        put('candidate_raw_unpack_cache_qualification', payloads['hash_raw_unpack_cache_qualification'])
        put('candidate_tile_prefetch_qualification', payloads['hash_tile_prefetch_qualification'])
        put('candidate_command_reuse_qualification', payloads['hash_command_reuse_qualification'])
        put('candidate_peer_phase_qualification', payloads['hash_peer_phase_qualification'])
        put('candidate_q4_nibble_qualification', payloads['hash_q4_nibble_qualification'])
        put('candidate_q4_direct_qualification', payloads['hash_q4_direct_qualification'])
        put('candidate_local_q4_qualification', payloads['hash_local_q4_qualification'])
        put('guardsource', payloads['guardsha'])
        if mode != '--bootstrap':
            put('guard', payloads['guardsha'])
        device = path(values['device'])
        device.parent.mkdir(parents=True, exist_ok=True)
        device.symlink_to('../../bin/tenstorrent_pci_probe')
        oldtag = values[oldtagkey]
        newtag = values[ending.replace('hash', 'tag', 1)]
        if mode in ('--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation'):
            assert values['tag_recurrent_deadline_validation'] == '2026102700010072'
            assert values['hash_recurrent_deadline_validation'] == '3e00974c4476f2755341fbcea034340652e976e292cb017344f4a70f896c652e'
        if mode in ('--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation'):
            assert values['tag_recurrent_prefix_validation'] == '2026102800010073'
            assert values['hash_recurrent_prefix_validation'] == '4ae7b6ea4217c0ee45303aab37e6793c9c17fe18a2acbbece8a7556332f92a3c'
        if mode in ('--apply-token-validation', '--restore-token-validation'):
            assert values['tag_token_validation'] == '2026103000010074'
            assert values['hash_token_validation'] == 'd78464058dec32de49e67c5263fca290751acdaf4252a7785ce56b9654e63aed'
        if mode in ('--apply-token-hold-recheck', '--restore-token-hold-recheck'):
            assert values['tag_token_hold_recheck'] == values['tag_token_validation'] == '2026103000010074'
            assert values['hash_token_hold_recheck'] == '781ab93300d1a8418cd752ae0728090c9e2c470a53320eafe22e4f3d3b8706d2'
            assert values['hash_scale_reuse_qualification'] == '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
            assert values['candidate_token_hold_recheck'] == values['stage'] + '/tenstorrent_pci_probe-token-hold-recheck-781ab933'
        if mode in ('--apply-token-root-frontend', '--restore-token-root-frontend'):
            assert values['tag_token_root_frontend'] == values['tag_token_hold_recheck'] == values['tag_token_validation'] == '2026103000010074'
            assert values['hash_token_root_frontend'] == '5f2fc63de2c4413d54b7087e5b923113246bc7a7f09747d0d79915bf950bf039'
            assert values['hash_scale_reuse_qualification'] == '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
            assert values['candidate_token_root_frontend'] == values['stage'] + '/tenstorrent_pci_probe-token-root-frontend-5f2fc63d'
        if mode in ('--apply-token-hidden-readout', '--restore-token-hidden-readout'):
            assert values['tag_token_hidden_readout'] == '2026103100010075'
            assert values['hash_token_hidden_readout'] == 'b74f4bfa5583aaf0cf8272d5136c73336c1db0f93e168e7212277f97705a92cc'
            assert values['candidate_token_hidden_readout'] == values['stage'] + '/tenstorrent_pci_probe-token-hidden-readout-b74f4bfa'
        if mode in ('--apply-token-fault-readout', '--restore-token-fault-readout'):
            assert values['tag_token_fault_readout'] == '2026103100010075'
            assert values['hash_token_fault_readout'] == 'cf40056394b5e3ecdac44d1679e26bdaf6256cc47ca8f2f6a890dc878a6d2906'
            assert values['candidate_token_fault_readout'] == values['stage'] + '/tenstorrent_pci_probe-token-fault-readout-cf400563'
        if mode in ('--apply-token-posted-cancel', '--restore-token-posted-cancel'):
            assert values['tag_token_posted_cancel'] == '2026103100010075'
            assert values['hash_token_posted_cancel'] == '18362bcb97180f8428b7e3b54aee7c81a42b8ed9f998aea11ccbefee5aff29af'
            assert values['candidate_token_posted_cancel'] == values['stage'] + '/tenstorrent_pci_probe-token-posted-cancel-18362bcb'
        if mode in ('--apply-token-precise-beta', '--restore-token-precise-beta'):
            assert values['tag_token_precise_beta'] == '2026103100010075'
            assert values['hash_token_precise_beta'] == '2723ac08ac75996f19f6e516c51eda615336f8dcfc3a8d6849f6d755da22c525'
            assert values['candidate_token_precise_beta'] == values['stage'] + '/tenstorrent_pci_probe-token-precise-beta-2723ac08'
        if mode in ('--apply-token-job-boot-fold', '--restore-token-job-boot-fold'):
            assert values['tag_token_job_boot_fold'] == '2026103100010075'
            assert values['hash_token_job_boot_fold'] == '44209b7873cd2e2f32f133b4c4c45a5870fe1669e98cd4165c4ebc51abdc5eaf'
            assert values['candidate_token_job_boot_fold'] == values['stage'] + '/tenstorrent_pci_probe-token-job-boot-fold-44209b78'
        if mode in ('--apply-token-decode-benchmark', '--restore-token-decode-benchmark'):
            assert values['tag_token_decode_benchmark'] == '2026103100010075'
            assert values['hash_token_decode_benchmark'] == '4ebc9463b80cfa73fe957800cadac819e7ff05e17ff7a2db7fd6362da2c43b14'
            assert values['candidate_token_decode_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-decode-benchmark-4ebc9463'
        if mode in ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            assert values['tag_token_frontier_benchmark'] == '2026103100010075'
            assert values['hash_token_frontier_benchmark'] == '9ad9e86bd5e45c5cb11dfe286f3528452dfe7b0e4630f67546acbb43a03064e8'
            assert values['candidate_token_frontier_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-frontier-benchmark-9ad9e86b'
        if mode in RETAINED_MODES:
            assert values['tag_token_retained_cohort_benchmark'] == '2026103100010075'
            assert values['hash_token_retained_cohort_benchmark'] == '1f3bcff838ff9f95d5f450c53000f02f80ddcd073aa5a5cc3018ee4208e0847c'
            assert values['candidate_token_retained_cohort_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-retained-cohort-benchmark-1f3bcff8'
        if mode in COMMIT_MODES:
            assert values['tag_token_commit_boundary_benchmark'] == '2026103100010075'
            assert values['hash_token_commit_boundary_benchmark'] == '78322cfa94db2519284efa12ad69cfa77981446f03081fc95aee2ee6234de8d1'
            assert values['candidate_token_commit_boundary_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-commit-boundary-benchmark-78322cfa'
        if mode in COHORT_SOURCE_MODES:
            assert values['tag_token_cohort_sources_benchmark'] == '2026103100010075'
            assert values['hash_token_cohort_sources_benchmark'] == '844ecfe2fc3b8396f0c9787c3a5458544abb91a91011d8f9844f3e137c3d03f5'
            assert values['candidate_token_cohort_sources_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-cohort-sources-benchmark-844ecfe2'
        if mode in PACKET_IDENTITY_MODES:
            assert values['tag_token_packet_identity_benchmark'] == '2026103100010075'
            assert values['hash_token_packet_identity_benchmark'] == 'ba9719dbc45ffb408d386c7c84da7322ae94136b6cbab26e39f6077870366830'
            assert values['candidate_token_packet_identity_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-packet-identity-benchmark-ba9719db'
        if mode in TID_BOUNDARY_MODES:
            assert values['tag_token_tid_boundary_benchmark'] == '2026103100010075'
            assert values['hash_token_tid_boundary_benchmark'] == 'e09cc07492d201b6a23f454ae0be514597051f8c55cf143b518eeb0b2f6c5e35'
            assert values['candidate_token_tid_boundary_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-tid-boundary-benchmark-e09cc074'
        if mode in COUNTER_GATHER_MODES:
            assert values['tag_token_counter_gather_benchmark'] == '2026103100010075'
            assert values['hash_token_counter_gather_benchmark'] == '0b8d1530b3f6de3462be23e923be169c82a8b15c76e83657c35c2d268b70405c'
            assert values['candidate_token_counter_gather_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-counter-gather-benchmark-0b8d1530'
        if mode in EMPTY_RECEIPT_POLL_MODES:
            assert values['tag_token_empty_receipt_poll_benchmark'] == '2026103100010075'
            assert values['hash_token_empty_receipt_poll_benchmark'] == '5a3ca6b662e3e7cb332a366c04b3650357a52e3bfe601cf0829e9e84e5e67008'
            assert values['candidate_token_empty_receipt_poll_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-empty-receipt-poll-benchmark-5a3ca6b6'
        if mode in FLAT_MATRIX_MODES:
            assert values['tag_token_flat_matrix_benchmark'] == '2026103100010075'
            assert values['hash_token_flat_matrix_benchmark'] == '8612962e38cab47999b8aecf67268c8e7fd651917b8a570fd79f79b78e1b165f'
            assert values['candidate_token_flat_matrix_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-flat-matrix-benchmark-8612962e'
        if mode in PREFETCH_MODES:
            assert values['tag_token_prefetch_benchmark'] == '2026103100010075'
            assert values['hash_token_prefetch_benchmark'] == '919d4a631ff664549a67bb2372ce2db344ba29473c452c0b273c4b27a2bdf978'
            assert values['candidate_token_prefetch_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-prefetch-benchmark-919d4a63'
        if mode in BATCHED_PREFETCH_MODES:
            assert values['tag_token_batched_prefetch_benchmark'] == '2026103100010075'
            assert values['hash_token_batched_prefetch_benchmark'] == '34322035d8aadcc8667d8a7ec3f2a144646f0060604e9e00ebfdd59fa7e4ca1c'
            assert values['candidate_token_batched_prefetch_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-batched-prefetch-benchmark-34322035'
        if mode in TWIN_BATCH_PREFETCH_MODES:
            assert values['tag_token_twin_batch_prefetch_benchmark'] == '2026103100010075'
            assert values['hash_token_twin_batch_prefetch_benchmark'] == '010c184ef474f962205918fd9a9a34b23b2ab47c3cf6a62d285fcb36d2e9e899'
            assert values['candidate_token_twin_batch_prefetch_benchmark'] == values['stage'] + '/tenstorrent_pci_probe-token-twin-batch-prefetch-benchmark-010c184e'
        status = f'P100A_RELOAD driver55 tag{oldtag} generation3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
        if fault == 'cold':
            status = status.replace('cold0', 'cold1')
        if fault == 'active':
            status = status.replace('active_owner0', 'active_owner1')
        if fault == 'open':
            status = status.replace('open_count1', 'open_count2')
        if fault == 'tag':
            status = status.replace(oldtag, '0123456789abcdef')
        if fault == 'old-tag71-metadata':
            status = status.replace(oldtag, values['tag_recurrent_block_validation'])
        if fault == 'old-tag72-metadata':
            status = status.replace(oldtag, values['tag_recurrent_deadline_validation'])
        if fault == 'generation':
            status = status.replace('generation3', 'generation0')
        if fault == 'quiesced':
            status = status.replace('quiesced0', 'quiesced1')
        if fault == 'guard-version':
            status = status.replace('guard1;', 'guard2;')
        if fault == 'duplicate-status':
            status += '\n' + status
        if fault == 'empty-status':
            status = ''
        client_body = ('#!/bin/sh\nset -eu\nprintf "%s\\n" "$1" >> ' + shlex.quote(str(root / 'calls')) + '\ncase "$1" in\n'
            ' --status) printf "%s\\n" ' + shlex.quote(status) + '; exit ' + ('1' if fault == 'status-error' else '0') + ';;\n'
            ' --quiesce) test "$2" = ' + shlex.quote(oldtag) + '; exit ' + ('1' if fault == 'quiesce-error' else '0') + ';;\n'
            ' --wait) test "$2" = ' + shlex.quote(newtag) + '; test "$3" = 0000000000000003; exit ' + ('1' if fault == 'wait-error' else '0') + ';;\n'
            ' *) exit 1;;\nesac\n').encode()
        client = put('client', client_body)
        client.chmod(0o700)
        if fault in values and (fault.startswith('candidate') or fault in ['guardsource', 'client', 'installed', 'guard']):
            path(values[fault]).write_bytes(b'wrong')
        if fault and fault.endswith('-link') and fault.removesuffix('-link') in values and (
                fault.startswith('candidate') or fault in ['client-link', 'guard-link', 'installed-link']):
            target = path(values[fault.removesuffix('-link')])
            real = target.with_name(target.name + '.real')
            target.rename(real)
            target.symlink_to(real.name)
        if fault == 'device-link':
            device.unlink()
            device.symlink_to('../../bin/wrong')
        if fault == 'stage-link':
            stage = path(values['stage'])
            real = stage.with_name(stage.name + '.real')
            stage.rename(real)
            stage.symlink_to(real.name)
        if fault in ['incoming', 'guardincoming']:
            put(fault, b'preexisting user file')
        hash_backup = mode in ('--apply-decoder-validation', '--restore-decoder-validation', '--apply-gdn-input-validation', '--restore-gdn-input-validation', '--apply-gdn-recurrent-validation', '--restore-gdn-recurrent-validation', '--apply-gdn-rendezvous-validation', '--restore-gdn-rendezvous-validation', '--apply-conv-silu-precision-validation', '--restore-conv-silu-precision-validation', '--apply-resident-output-validation', '--restore-resident-output-validation', '--apply-resident-layer-validation', '--restore-resident-layer-validation')
        hash_backup |= mode in ('--apply-mixed-live-validation', '--restore-mixed-live-validation')
        hash_backup |= mode in ('--apply-recurrent-block-validation', '--restore-recurrent-block-validation')
        hash_backup |= mode in ('--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation')
        hash_backup |= mode in ('--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation')
        hash_backup |= mode in ('--apply-token-validation', '--restore-token-validation')
        hash_backup |= mode in ('--apply-token-hold-recheck', '--restore-token-hold-recheck')
        hash_backup |= mode in ('--apply-token-root-frontend', '--restore-token-root-frontend')
        hash_backup |= mode in ('--apply-token-hidden-readout', '--restore-token-hidden-readout')
        hash_backup |= mode in ('--apply-token-fault-readout', '--restore-token-fault-readout')
        hash_backup |= mode in ('--apply-token-posted-cancel', '--restore-token-posted-cancel')
        hash_backup |= mode in ('--apply-token-precise-beta', '--restore-token-precise-beta')
        hash_backup |= mode in ('--apply-token-job-boot-fold', '--restore-token-job-boot-fold')
        hash_backup |= mode in ('--apply-token-decode-benchmark', '--restore-token-decode-benchmark')
        hash_backup |= mode in ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
        hash_backup |= mode in RETAINED_MODES
        hash_backup |= mode in COMMIT_MODES
        hash_backup |= mode in COHORT_SOURCE_MODES
        hash_backup |= mode in PACKET_IDENTITY_MODES
        hash_backup |= mode in TID_BOUNDARY_MODES
        hash_backup |= mode in COUNTER_GATHER_MODES
        hash_backup |= mode in EMPTY_RECEIPT_POLL_MODES
        hash_backup |= mode in FLAT_MATRIX_MODES
        hash_backup |= mode in PREFETCH_MODES
        hash_backup |= mode in BATCHED_PREFETCH_MODES
        hash_backup |= mode in TWIN_BATCH_PREFETCH_MODES
        backup_name = 'qualified55-before-hot-reload' if mode == '--bootstrap' else 'qualified-reload-' + oldtag
        if hash_backup:
            backup_name += '-' + hashlib.sha256(payloads[starting]).hexdigest()
        backup_target = path(values['stage']) / backup_name
        if legacy_backup or qualified_backup:
            assert hash_backup
        legacy_target = path(values['stage']) / ('qualified-reload-' + oldtag)
        legacy_bytes = b'previous byte-distinct DEBUG44 backup;MUST remain untouched'
        if legacy_backup:
            legacy_target.write_bytes(legacy_bytes)
        backup_inode = None
        if qualified_backup:
            backup_target.write_bytes(payloads[starting])
            backup_inode = backup_target.stat().st_ino
        other_token_records = []
        if other_token_backup:
            assert mode in TOKEN_MODES and mode.startswith('--restore-')
            for revision in TOKEN_REVISIONS:
                other_key = 'hash_' + revision
                if other_key == starting:
                    continue
                other_bytes = payloads[other_key]
                other_target = path(values['stage']) / ('qualified-reload-' + oldtag + '-' + hashlib.sha256(other_bytes).hexdigest())
                assert other_target != backup_target
                other_target.write_bytes(other_bytes)
                other_token_records.append((other_target, other_bytes, other_target.stat().st_ino))
            assert len(other_token_records) == 2
        if fault == 'backup':
            backup_target.write_bytes(b'preexisting backup')
        if fault == 'backup-link':
            assert hash_backup
            real = backup_target.with_name(backup_target.name + '.real')
            real.write_bytes(payloads[starting])
            backup_target.symlink_to(real.name)
        if fault in ('previous-debug44', 'previous-capture-debug44'):
            # Same numeric tag is NOT byte identity. A new updater must
            # reject both the previous candidate and its installed image,
            # before opening the admin device or modifying the guard.
            assert mode in ('--apply-decoder-validation', '--restore-decoder-validation')
            previous = (b'test-d96899a74c6949a22caec805dbd611cee80f2ff96c909758d772557d1a4a0c0a'
                        if fault == 'previous-debug44' else b'test-4db734553910748e51c22a5d2017b01d2fa2c42ea1409aabf9b55839732d6edd')
            if mode == '--apply-decoder-validation':
                put('candidate_decoder_validation', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('previous-layer69', 'previous-capture-layer69'):
            # Same role/tag does NOT identify frontend-policy bytes.
            # Refuse BOTH old images BEFORE any administrative open.
            assert mode in ('--apply-resident-layer-validation', '--restore-resident-layer-validation')
            previous = (b'test-beeaf6b0c5eb5af1f1992a357abc0ceefcc3614aa2f81741d8eb86a432b094d2'
                        if fault == 'previous-layer69' else b'test-960971c38107ccc707cb0cdeb7f04a9b4cddc053c3f6e429f94e7dfc21518d8f')
            if mode == '--apply-resident-layer-validation':
                put('candidate_resident_layer_validation', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'previous-debug71':
            assert mode in ('--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation')
            if mode == '--apply-recurrent-deadline-validation':
                put('candidate_recurrent_deadline_validation', payloads['hash_recurrent_block_validation'])
            else:
                installed.write_bytes(payloads['hash_recurrent_block_validation'])
        if fault in ('prefix-previous-debug71', 'prefix-previous-debug72', 'prefix-same-tag-wrong-bytes'):
            assert mode in ('--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation')
            previous = {'prefix-previous-debug71': payloads['hash_recurrent_block_validation'],
                        'prefix-previous-debug72': payloads['hash_recurrent_deadline_validation'],
                        'prefix-same-tag-wrong-bytes': b'unqualified distinct bytes with a claimed DEBUG73 tag'}[fault]
            if mode == '--apply-recurrent-prefix-validation':
                put('candidate_recurrent_prefix_validation', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('token-same-tag-wrong-bytes', 'token-other-revision', 'token-third-revision'):
            assert mode in TOKEN_MODES
            revision = mode.removeprefix('--apply-').removeprefix('--restore-').replace('-', '_')
            others = [item for item in TOKEN_REVISIONS if item != revision]
            assert len(others) == 2
            wrong_bytes = (b'unqualified distinct bytes claiming tag74' if fault == 'token-same-tag-wrong-bytes'
                           else payloads['hash_' + others[fault == 'token-third-revision']])
            if mode.startswith('--apply-'):
                put('candidate_' + revision, wrong_bytes)
            else:
                installed.write_bytes(wrong_bytes)
        if fault in ('fault-old-tag75-bytes','fault-old-tag74-bytes'):
            assert mode in ('--apply-token-fault-readout','--restore-token-fault-readout')
            previous=payloads['hash_token_hidden_readout' if fault=='fault-old-tag75-bytes' else 'hash_token_root_frontend']
            if mode.startswith('--apply-'):put('candidate_token_fault_readout',previous)
            else:installed.write_bytes(previous)
        if fault in ('posted-old-fault', 'posted-old-hidden', 'posted-old-tag74', 'posted-same-tag-wrong'):
            assert mode in ('--apply-token-posted-cancel', '--restore-token-posted-cancel')
            previous = (b'unqualified distinct bytes claiming tag75' if fault == 'posted-same-tag-wrong'
                        else payloads[{'posted-old-fault': 'hash_token_fault_readout',
                                       'posted-old-hidden': 'hash_token_hidden_readout',
                                       'posted-old-tag74': 'hash_token_root_frontend'}[fault]])
            if mode.startswith('--apply-'):
                put('candidate_token_posted_cancel', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('beta-old-posted', 'beta-old-fault', 'beta-old-hidden', 'beta-old-tag74', 'beta-same-tag-wrong'):
            assert mode in ('--apply-token-precise-beta', '--restore-token-precise-beta')
            previous = (b'unqualified distinct bytes claiming tag75' if fault == 'beta-same-tag-wrong'
                        else payloads[{'beta-old-posted': 'hash_token_posted_cancel',
                                       'beta-old-fault': 'hash_token_fault_readout',
                                       'beta-old-hidden': 'hash_token_hidden_readout',
                                       'beta-old-tag74': 'hash_token_root_frontend'}[fault]])
            if mode.startswith('--apply-'):
                put('candidate_token_precise_beta', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('fold-old-beta', 'fold-old-posted', 'fold-old-fault', 'fold-old-hidden', 'fold-old-tag74', 'fold-same-tag-wrong'):
            assert mode in ('--apply-token-job-boot-fold', '--restore-token-job-boot-fold')
            previous = (b'unqualified distinct bytes claiming tag75' if fault == 'fold-same-tag-wrong'
                        else payloads[{'fold-old-beta': 'hash_token_precise_beta',
                                       'fold-old-posted': 'hash_token_posted_cancel',
                                       'fold-old-fault': 'hash_token_fault_readout',
                                       'fold-old-hidden': 'hash_token_hidden_readout',
                                       'fold-old-tag74': 'hash_token_root_frontend'}[fault]])
            if mode.startswith('--apply-'):
                put('candidate_token_job_boot_fold', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('decode-old-fold', 'decode-old-beta', 'decode-old-posted', 'decode-old-fault', 'decode-old-hidden', 'decode-old-tag74', 'decode-same-tag-wrong'):
            assert mode in ('--apply-token-decode-benchmark', '--restore-token-decode-benchmark')
            previous = (b'unknown bytes claiming the same tag75' if fault == 'decode-same-tag-wrong'
                        else payloads[{'decode-old-fold': 'hash_token_job_boot_fold',
                                       'decode-old-beta': 'hash_token_precise_beta',
                                       'decode-old-posted': 'hash_token_posted_cancel',
                                       'decode-old-fault': 'hash_token_fault_readout',
                                       'decode-old-hidden': 'hash_token_hidden_readout',
                                       'decode-old-tag74': 'hash_token_root_frontend'}[fault]])
            if mode.startswith('--apply-'):
                put('candidate_token_decode_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('frontier-old-decode', 'frontier-old-fold', 'frontier-old-beta', 'frontier-old-posted', 'frontier-old-fault', 'frontier-old-hidden', 'frontier-old-tag74', 'frontier-same-tag-wrong'):
            assert mode in ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark') + RETAINED_MODES + COMMIT_MODES + COHORT_SOURCE_MODES + PACKET_IDENTITY_MODES + TID_BOUNDARY_MODES + COUNTER_GATHER_MODES + EMPTY_RECEIPT_POLL_MODES + FLAT_MATRIX_MODES + PREFETCH_MODES
            previous = (b'unknown frontier bytes claiming the same tag75' if fault == 'frontier-same-tag-wrong'
                        else payloads[{'frontier-old-decode': 'hash_token_decode_benchmark',
                                       'frontier-old-fold': 'hash_token_job_boot_fold',
                                       'frontier-old-beta': 'hash_token_precise_beta',
                                       'frontier-old-posted': 'hash_token_posted_cancel',
                                       'frontier-old-fault': 'hash_token_fault_readout',
                                       'frontier-old-hidden': 'hash_token_hidden_readout',
                                       'frontier-old-tag74': 'hash_token_root_frontend'}[fault]])
            if mode.startswith('--apply-'):
                put('candidate_token_prefetch_benchmark' if mode in PREFETCH_MODES else 'candidate_token_flat_matrix_benchmark' if mode in FLAT_MATRIX_MODES else 'candidate_token_empty_receipt_poll_benchmark' if mode in EMPTY_RECEIPT_POLL_MODES else 'candidate_token_counter_gather_benchmark' if mode in COUNTER_GATHER_MODES else 'candidate_token_tid_boundary_benchmark' if mode in TID_BOUNDARY_MODES else 'candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_frontier_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'retained-old-frontier':
            assert mode in RETAINED_MODES
            previous = payloads['hash_token_frontier_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_token_retained_cohort_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('commit-old-frontier', 'commit-old-retained', 'retained-new-commit'):
            assert mode in (RETAINED_MODES if fault == 'retained-new-commit' else COMMIT_MODES)
            previous = payloads[{'commit-old-frontier': 'hash_token_frontier_benchmark',
                                 'commit-old-retained': 'hash_token_retained_cohort_benchmark',
                                 'retained-new-commit': 'hash_token_commit_boundary_benchmark'}[fault]]
            if mode.startswith('--apply-'):
                put('candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_commit_boundary_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('cohort-old-commit', 'cohort-old-retained', 'cohort-old-frontier'):
            assert mode in COHORT_SOURCE_MODES
            previous = payloads[{'cohort-old-commit': 'hash_token_commit_boundary_benchmark',
                                 'cohort-old-retained': 'hash_token_retained_cohort_benchmark',
                                 'cohort-old-frontier': 'hash_token_frontier_benchmark'}[fault]]
            if mode.startswith('--apply-'):
                put('candidate_token_cohort_sources_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-cohort':
            assert mode in COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous = payloads['hash_token_cohort_sources_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_frontier_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('packet-old-cohort', 'packet-old-commit', 'packet-old-retained', 'packet-old-frontier'):
            assert mode in PACKET_IDENTITY_MODES
            previous = payloads[{'packet-old-cohort': 'hash_token_cohort_sources_benchmark',
                                 'packet-old-commit': 'hash_token_commit_boundary_benchmark',
                                 'packet-old-retained': 'hash_token_retained_cohort_benchmark',
                                 'packet-old-frontier': 'hash_token_frontier_benchmark'}[fault]]
            if mode.startswith('--apply-'):
                put('candidate_token_packet_identity_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-packet':
            assert mode in COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous = payloads['hash_token_packet_identity_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_frontier_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('tid-old-packet', 'tid-old-cohort', 'tid-old-commit', 'tid-old-retained', 'tid-old-frontier'):
            assert mode in TID_BOUNDARY_MODES
            previous = payloads[{'tid-old-packet': 'hash_token_packet_identity_benchmark',
                                 'tid-old-cohort': 'hash_token_cohort_sources_benchmark',
                                 'tid-old-commit': 'hash_token_commit_boundary_benchmark',
                                 'tid-old-retained': 'hash_token_retained_cohort_benchmark',
                                 'tid-old-frontier': 'hash_token_frontier_benchmark'}[fault]]
            if mode.startswith('--apply-'):
                put('candidate_token_tid_boundary_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-tid':
            assert mode in PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous = payloads['hash_token_tid_boundary_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_frontier_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault in ('gather-old-tid', 'gather-old-packet', 'gather-old-cohort', 'gather-old-commit', 'gather-old-retained', 'gather-old-frontier'):
            assert mode in COUNTER_GATHER_MODES
            previous = payloads[{'gather-old-tid': 'hash_token_tid_boundary_benchmark',
                                 'gather-old-packet': 'hash_token_packet_identity_benchmark',
                                 'gather-old-cohort': 'hash_token_cohort_sources_benchmark',
                                 'gather-old-commit': 'hash_token_commit_boundary_benchmark',
                                 'gather-old-retained': 'hash_token_retained_cohort_benchmark',
                                 'gather-old-frontier': 'hash_token_frontier_benchmark'}[fault]]
            if mode.startswith('--apply-'):
                put('candidate_token_counter_gather_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-gather':
            assert mode in TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous = payloads['hash_token_counter_gather_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_token_tid_boundary_benchmark' if mode in TID_BOUNDARY_MODES else 'candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES else 'candidate_token_frontier_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault and fault.startswith('empty-old-'):
            assert mode in EMPTY_RECEIPT_POLL_MODES
            key = {'empty-old-gather': 'hash_token_counter_gather_benchmark',
                   'empty-old-tid': 'hash_token_tid_boundary_benchmark',
                   'empty-old-packet': 'hash_token_packet_identity_benchmark',
                   'empty-old-cohort': 'hash_token_cohort_sources_benchmark',
                   'empty-old-commit': 'hash_token_commit_boundary_benchmark',
                   'empty-old-retained': 'hash_token_retained_cohort_benchmark',
                   'empty-old-frontier': 'hash_token_frontier_benchmark'}[fault]
            if mode.startswith('--apply-'):
                put('candidate_token_empty_receipt_poll_benchmark', payloads[key])
            else:
                installed.write_bytes(payloads[key])
        if fault == 'old-route-new-empty':
            assert mode in COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous = payloads['hash_token_empty_receipt_poll_benchmark']
            if mode.startswith('--apply-'):
                key = ('candidate_token_counter_gather_benchmark' if mode in COUNTER_GATHER_MODES
                       else 'candidate_token_tid_boundary_benchmark' if mode in TID_BOUNDARY_MODES
                       else 'candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES
                       else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES
                       else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES
                       else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES
                       else 'candidate_token_frontier_benchmark')
                put(key, previous)
            else:
                installed.write_bytes(previous)
        if fault and fault.startswith('flat-old-'):
            assert mode in FLAT_MATRIX_MODES
            key = {'flat-old-empty': 'hash_token_empty_receipt_poll_benchmark',
                   'flat-old-gather': 'hash_token_counter_gather_benchmark',
                   'flat-old-tid': 'hash_token_tid_boundary_benchmark',
                   'flat-old-packet': 'hash_token_packet_identity_benchmark',
                   'flat-old-cohort': 'hash_token_cohort_sources_benchmark',
                   'flat-old-commit': 'hash_token_commit_boundary_benchmark',
                   'flat-old-retained': 'hash_token_retained_cohort_benchmark',
                   'flat-old-frontier': 'hash_token_frontier_benchmark'}[fault]
            if mode.startswith('--apply-'):
                put('candidate_token_flat_matrix_benchmark', payloads[key])
            else:
                installed.write_bytes(payloads[key])
        if fault == 'old-route-new-flat':
            assert mode in EMPTY_RECEIPT_POLL_MODES + COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
            previous=payloads['hash_token_flat_matrix_benchmark']
            if mode.startswith('--apply-'):
                key=('candidate_token_empty_receipt_poll_benchmark' if mode in EMPTY_RECEIPT_POLL_MODES
                     else 'candidate_token_counter_gather_benchmark' if mode in COUNTER_GATHER_MODES
                     else 'candidate_token_tid_boundary_benchmark' if mode in TID_BOUNDARY_MODES
                     else 'candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES
                     else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES
                     else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES
                     else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES
                     else 'candidate_token_frontier_benchmark')
                put(key,previous)
            else:
                installed.write_bytes(previous)
        if fault and fault.startswith('prefetch-old-'):
            assert mode in PREFETCH_MODES
            revision=fault.removeprefix('prefetch-old-')
            key={'flat':'hash_token_flat_matrix_benchmark','empty':'hash_token_empty_receipt_poll_benchmark',
                 'gather':'hash_token_counter_gather_benchmark','tid':'hash_token_tid_boundary_benchmark',
                 'packet':'hash_token_packet_identity_benchmark','cohort':'hash_token_cohort_sources_benchmark',
                 'commit':'hash_token_commit_boundary_benchmark','retained':'hash_token_retained_cohort_benchmark',
                 'frontier':'hash_token_frontier_benchmark'}[revision]
            if mode.startswith('--apply-'):put('candidate_token_prefetch_benchmark',payloads[key])
            else:installed.write_bytes(payloads[key])
        if fault == 'old-route-new-prefetch':
            assert mode in FLAT_MATRIX_MODES + EMPTY_RECEIPT_POLL_MODES + COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark','--restore-token-frontier-benchmark')
            previous=payloads['hash_token_prefetch_benchmark']
            if mode.startswith('--apply-'):
                key=('candidate_token_flat_matrix_benchmark' if mode in FLAT_MATRIX_MODES
                     else 'candidate_token_empty_receipt_poll_benchmark' if mode in EMPTY_RECEIPT_POLL_MODES
                     else 'candidate_token_counter_gather_benchmark' if mode in COUNTER_GATHER_MODES
                     else 'candidate_token_tid_boundary_benchmark' if mode in TID_BOUNDARY_MODES
                     else 'candidate_token_packet_identity_benchmark' if mode in PACKET_IDENTITY_MODES
                     else 'candidate_token_cohort_sources_benchmark' if mode in COHORT_SOURCE_MODES
                     else 'candidate_token_commit_boundary_benchmark' if mode in COMMIT_MODES
                     else 'candidate_token_retained_cohort_benchmark' if mode in RETAINED_MODES
                     else 'candidate_token_frontier_benchmark')
                put(key,previous)
            else:installed.write_bytes(previous)
        if fault and fault.startswith('batched-old-'):
            assert mode in BATCHED_PREFETCH_MODES
            revision = fault.removeprefix('batched-old-')
            if revision == 'same-tag-wrong':
                previous = b'unknown batched bytes claiming the same tag75'
            else:
                key = {'prefetch': 'hash_token_prefetch_benchmark', 'flat': 'hash_token_flat_matrix_benchmark',
                       'empty': 'hash_token_empty_receipt_poll_benchmark', 'gather': 'hash_token_counter_gather_benchmark',
                       'tid': 'hash_token_tid_boundary_benchmark', 'packet': 'hash_token_packet_identity_benchmark',
                       'cohort': 'hash_token_cohort_sources_benchmark', 'commit': 'hash_token_commit_boundary_benchmark',
                       'retained': 'hash_token_retained_cohort_benchmark', 'frontier': 'hash_token_frontier_benchmark'}[revision]
                previous = payloads[key]
            if mode.startswith('--apply-'):
                put('candidate_token_batched_prefetch_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-batched':
            assert mode in BATCHED_PREFETCH_OLD_MODES
            previous = payloads['hash_token_batched_prefetch_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_' + mode.removeprefix('--apply-').replace('-', '_'), previous)
            else:
                installed.write_bytes(previous)
        if fault and fault.startswith('twin-old-'):
            assert mode in TWIN_BATCH_PREFETCH_MODES
            revision = fault.removeprefix('twin-old-')
            assert revision == 'same-tag-wrong' or revision in TWIN_BATCH_PREFETCH_OLD_REVISIONS
            previous = (b'unknown twin bytes claiming the same tag75' if revision == 'same-tag-wrong'
                        else payloads['hash_' + revision])
            if mode.startswith('--apply-'):
                put('candidate_token_twin_batch_prefetch_benchmark', previous)
            else:
                installed.write_bytes(previous)
        if fault == 'old-route-new-twin':
            assert mode in TWIN_BATCH_PREFETCH_OLD_MODES
            previous = payloads['hash_token_twin_batch_prefetch_benchmark']
            if mode.startswith('--apply-'):
                put('candidate_' + mode.removeprefix('--apply-').replace('-', '_'), previous)
            else:
                installed.write_bytes(previous)
        before = installed.read_bytes()
        if fault == 'previous-debug52':
            assert mode in ('--apply-conv-silu-precision-validation', '--restore-conv-silu-precision-validation')
            installed.write_bytes(payloads['hash_gdn_rendezvous_validation']);before=installed.read_bytes()
        images = ['KERNEL TEAM:', '1 0x0 0x0 0 0 kernel_riscv64'] + [f'{i} 0x0 0x0 0 0 module{i}' for i in range(2, 52)]
        if fault == 'loaded':
            images.append('999 0x0 0x0 0 0 tenstorrent_pci_probe')
        teams = ['Id Team', '1 kernel_team', '42 /bin/registrar']
        owner_faults = {
            'owner': './inference-vector-21e18126',
            'owner-assistant': '/boot/system/apps/Assistant',
            'owner-bare': 'Assistant',
            'owner-truncated': '/boot/home/pioneer-q',
            'owner-qwen': './pioneer-qwen38-bench',
            'owner-llama': './llama-cli',
            'owner-tt': '/boot/home/tenstorrent_haiku_mmio_probe',
            'owner-underscore': '/boot/home/pioneer_q38_client',
            'owner-csr': '/boot/home/pioneer-p100a-hot-reload-20261019-a/graph-csr-debug-b4eef9d3',
            'owner-ffn-code': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-ffn-code-debug-test',
            'owner-ffn-peer-lifetime': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-peer-lifetime-debug-36222fe9',
            'owner-ffn-root-lifetime': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-root-lifetime-debug-c33b9c98',
            'owner-ffn-model-init': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-ffn-model-init-f4fc646d',
            'owner-ffn-native-graph': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-ffn-native-graph-35add96a',
            'owner-ffn-host-profile': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-ffn-host-profile-2e2086db',
            'owner-ffn-sealed-proof': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-ffn-sealed-proof-991155b6',
            'owner-embedding-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-embedding-validation-ed5742b2',
            'owner-rms-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-rms-validation-20d93bef',
            'owner-projection-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-projection-validation-93992ae6',
            'owner-decoder-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-resident-decoder-validation-f3b54e8b',
            'owner-gdn-input-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-gdn-input-validation-d9da6825',
            'owner-gdn-input-validation-underscore': '/boot/home/pioneer_q38_gdn_input_validation',
            'owner-gdn-recurrent-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-gdn-recurrent-validation-50f30dee',
            'owner-gdn-rendezvous-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-gdn-rendezvous-validation-83a8caff',
            'owner-gdn-recurrent-validation-underscore': '/boot/home/pioneer_q38_gdn_recurrent_validation',
            'owner-gdn-rendezvous-validation-underscore': '/boot/home/pioneer_q38_gdn_rendezvous_validation',
            'owner-conv-precision-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-conv-silu-precision-validation-cb7e8f5f',
            'owner-conv-precision-qualified': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-conv-silu-precision-validation-495b9e18',
            'owner-conv-precision-validation-underscore': '/boot/home/pioneer_q38_conv_silu_precision_validation',
            'owner-resident-output-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-resident-output-validation-0e38765f',
            'owner-resident-output-validation-underscore': '/boot/home/pioneer_q38_resident_output_validation',
            'owner-resident-layer-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-resident-layer-validation-057b1eca',
            'owner-resident-layer-validation-underscore': '/boot/home/pioneer_q38_resident_layer_validation',
            'owner-recurrent-block-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-recurrent-DEBUG71-VALIDATION-ONLY',
            'owner-recurrent-block-validation-underscore': '/boot/home/pioneer_q38_recurrent_block_validation',
            'owner-recurrent-deadline-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-recurrent-DEBUG72-VALIDATION-ONLY',
            'owner-recurrent-prefix-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-prefix-DEBUG73-VALIDATION-ONLY',
            'owner-recurrent-prefix-validation-underscore': '/boot/home/pioneer_q38_recurrent_prefix_validation',
            'owner-token-validation': '/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-q38-token-DEBUG74-DIAGNOSTIC-ONLY',
            'owner-token-hidden-validation': '/boot/home/pioneer-q38-hidden75-client-20261031-1a061f7a/pioneer-q38-token-DEBUG75-DIAGNOSTIC-ONLY',
            'owner-token-validation-underscore': '/boot/home/pioneer_q38_token_validation',
            'owner-token-source-check': '/boot/home/pioneer-token-source-fw-1593cbab/source-check --validate-cancel-boundary',
        }
        if fault in owner_faults:
            teams.append('999 ' + owner_faults[fault])
        if fault == 'ps-empty':
            teams = []
        if fault == 'ps-header-only':
            teams = ['Id Team']
        if fault == 'ps-no-header':
            teams = teams[1:]
        if fault == 'ps-duplicate-header':
            teams.append('Id Team')
        if fault == 'ps-malformed':
            teams.append('partial inventory error')
        team_body = 'printf "%s\\n" ' + ' '.join(map(shlex.quote, teams)) + ';'
        if fault == 'ps-error':
            team_body = 'return 1;'
        if fault == 'owner-late':
            counter = shlex.quote(str(root / 'ps-count'))
            team_body = ('n=0; if test -f ' + counter + '; then read -r n < ' + counter + '; fi; '
                'n=$((n+1)); printf "%s\\n" "$n" > ' + counter + '; ' + team_body
                + ' if test "$n" -ge 2; then printf "%s\\n" "999 ./inference-auth-parent"; fi;')
        mocks = '\n'.join([
            'uname() { printf "%s\\n" ' + ('Linux' if fault == 'platform' else 'Haiku') + '; }',
            'listimage() { printf "%s\\n" ' + ' '.join(map(shlex.quote, images)) + '; }',
            'ps() { ' + team_body + ' }',
            'sha256sum() { ' + shlex.quote(shutil.which('shasum')) + ' -a 256 "$@"; }',
            'sync() { :; }',
        ])
        rewritten = text.replace('/boot/home', str(root / 'boot/home'))
        for key, payload in payloads.items():
            rewritten = rewritten.replace(values[key], hashlib.sha256(payload).hexdigest())
        rewritten = rewritten.replace(values['clientsha'], hashlib.sha256(client_body).hexdigest())
        local = root / 'actual-updater.sh'
        local.write_text(mocks + '\n' + rewritten)
        result = subprocess.run(['/bin/sh', str(local), mode], capture_output=True, text=True, timeout=15)
        if legacy_backup:
            assert legacy_target.read_bytes() == legacy_bytes, (mode, fault, 'PREVIOUS_BACKUP_OVERWRITTEN')
        if qualified_backup:
            assert backup_target.stat().st_ino == backup_inode, (mode, fault, 'VALID_BACKUP_REPLACED')
        for other_target, other_bytes, other_inode in other_token_records:
            assert other_target.read_bytes() == other_bytes and other_target.stat().st_ino == other_inode, (mode, fault, 'OTHER_REVISION_BACKUP_CHANGED')
        if fault:
            assert result.returncode != 0, (mode, fault, result.stdout, result.stderr)
            if fault.startswith('batched-old-') or fault == 'old-route-new-batched':
                assert not (root / 'calls').exists(), (mode, fault, 'ADMIN_OPEN_WITH_WRONG_BATCHED_REVISION')
            if fault.startswith('twin-old-') or fault == 'old-route-new-twin':
                assert not (root / 'calls').exists(), (mode, fault, 'ADMIN_OPEN_WITH_WRONG_TWIN_REVISION')
            if fault in ('packet-old-cohort', 'packet-old-commit', 'packet-old-retained', 'packet-old-frontier', 'old-route-new-packet'):
                assert not (root / 'calls').exists(), (mode, fault, 'ADMIN_OPEN_WITH_DIFFERENT_BYTE_REVISION')
            if fault in ('previous-debug44', 'previous-capture-debug44', 'previous-debug52', 'previous-layer69', 'previous-capture-layer69', 'previous-debug71', 'token-same-tag-wrong-bytes', 'token-other-revision', 'token-third-revision'):
                assert not (root / 'calls').exists(), (mode, fault, 'ADMIN_OPEN_WITH_PREVIOUS_IMAGE')
            if fault in ('backup', 'backup-link') and mode != '--bootstrap':
                assert (root / 'calls').read_text().splitlines() == ['--status'], (mode, fault, 'QUIESCED_WITH_UNPROVEN_BACKUP')
            if fault in owner_faults or fault.startswith('ps-'):
                assert not (root / 'calls').exists(), (mode, fault, 'ADMIN_OPEN_DURING_UNPROVEN_PARENT')
            if fault == 'owner-late' and mode != '--bootstrap':
                assert (root / 'calls').read_text().splitlines() == ['--status'], (mode, fault, 'QUIESCED_DURING_PARENT_LIFETIME')
            if fault != 'wait-error':
                assert installed.read_bytes() == before, (mode, fault, 'REPLACED_DESPITE_GUARD_FAILURE')
            else:
                assert installed.read_bytes() == payloads[ending]
                assert 'NO_AUTOMATIC_ROLLBACK_OR_RESET' in result.stdout
        else:
            assert result.returncode == 0, (mode, result.stdout, result.stderr)
            assert installed.read_bytes() == payloads[ending]
            if hash_backup:
                assert backup_target.read_bytes() == payloads[starting], (mode, 'MISSING_EXACT_HASH_BACKUP')
            if mode != '--bootstrap':
                assert (root / 'calls').read_text().splitlines() == ['--status', '--quiesce', '--wait']


if __name__ == '__main__':
    count = 0
    selected = set(sys.argv[1:])
    seen = set()
    retired_modes = {
        '--apply-recurrent-block-validation': 'STOP_RECURRENT71_FAILED_NATIVE',
        '--apply-token-batched-prefetch-benchmark': 'STOP_BATCHED_PREFETCH34322_FAILED_PHYSICAL',
    }
    for retired, reason in retired_modes.items():
        if not selected or retired in selected:
            stopped = subprocess.run(['/bin/sh', str(SCRIPT), retired], capture_output=True, text=True, timeout=5)
            assert stopped.returncode == 2 and not stopped.stdout and reason in stopped.stderr
            count += 1
            seen.add(retired)
    common = ['candidatea', 'candidateb', 'guardsource', 'client', 'installed', 'candidatea-link', 'client-link',
              'installed-link', 'device-link', 'stage-link', 'incoming', 'guardincoming', 'backup', 'platform']
    hot = ['guard', 'guard-link', 'cold', 'active', 'open', 'tag', 'generation', 'quiesced', 'guard-version',
           'duplicate-status', 'empty-status', 'status-error', 'quiesce-error', 'wait-error']
    parents = ['owner', 'owner-assistant', 'owner-bare', 'owner-truncated', 'owner-qwen',
               'owner-llama', 'owner-tt', 'owner-underscore', 'owner-late', 'ps-error',
               'ps-empty', 'ps-header-only', 'ps-no-header', 'ps-duplicate-header', 'ps-malformed', 'owner-csr', 'owner-ffn-code', 'owner-ffn-peer-lifetime', 'owner-ffn-root-lifetime', 'owner-ffn-model-init', 'owner-ffn-native-graph', 'owner-ffn-host-profile', 'owner-ffn-sealed-proof']
    modes = ['--bootstrap', '--apply-a', '--apply-b', '--apply-profile', '--restore-profile', '--apply-batch', '--restore-batch', '--apply-compact', '--restore-compact', '--apply-fast', '--restore-fast', '--apply-reuse', '--restore-reuse', '--apply-cross-reuse', '--restore-cross-reuse', '--apply-work', '--restore-work', '--apply-work-profile', '--restore-work-profile', '--apply-work-posted', '--restore-work-posted', '--apply-work-cohort', '--restore-work-cohort', '--apply-work-q4', '--restore-work-q4', '--apply-work-q4-inline', '--restore-work-q4-inline'] + ['--apply-work-span', '--restore-work-span', '--apply-work-fast-span', '--restore-work-fast-span', '--apply-work-span-code-reuse', '--restore-work-span-code-reuse', '--apply-work-int-sfpu', '--restore-work-int-sfpu', '--apply-work-pipelined', '--restore-work-pipelined', '--apply-work-static-vector', '--restore-work-static-vector', '--apply-work-cached-affine', '--restore-work-cached-affine', '--apply-work-replay-operand', '--restore-work-replay-operand', '--apply-work-dot', '--restore-work-dot', '--apply-work-output-dma', '--restore-work-output-dma', '--apply-work-queued-entry', '--restore-work-queued-entry', '--apply-local-delta', '--restore-local-delta', '--apply-local-delta-terminal', '--restore-local-delta-terminal', '--apply-current-profile', '--restore-current-profile']
    modes += ['--apply-sparse-profile', '--restore-sparse-profile', '--apply-sparse-export', '--restore-sparse-export']
    modes += ['--apply-normal-clock', '--restore-normal-clock']
    modes += ['--apply-input-sum-cache', '--restore-input-sum-cache']
    modes += ['--apply-zero-minimum', '--restore-zero-minimum']
    modes += ['--apply-native-operand', '--restore-native-operand']
    modes += ['--apply-compact-output', '--restore-compact-output']
    modes += ['--apply-compact-profile', '--restore-compact-profile']
    modes += ['--apply-deferred-affine', '--restore-deferred-affine']
    modes += ['--apply-ordered-extents', '--restore-ordered-extents']
    modes += ['--apply-inline-config', '--restore-inline-config']
    modes += ['--apply-postscale-delta', '--restore-postscale-delta']
    modes += ['--apply-gdn-zero-diagnostic', '--restore-gdn-zero-diagnostic']
    modes += ['--apply-resident-root-diagnostic', '--restore-resident-root-diagnostic']
    modes += ['--apply-root-cycle-diagnostic', '--restore-root-cycle-diagnostic']
    modes += ['--apply-block-affine-qualification', '--restore-block-affine-qualification']
    modes += ['--apply-scale-reuse-qualification', '--restore-scale-reuse-qualification']
    modes += ['--apply-group32-qualification', '--restore-group32-qualification']
    modes += ['--apply-native-q8-qualification', '--restore-native-q8-qualification']
    modes += ['--apply-affine-rawdot-qualification', '--restore-affine-rawdot-qualification']
    modes += ['--apply-direct-srcb-qualification', '--restore-direct-srcb-qualification']
    modes += ['--apply-sentinel-poll-qualification', '--restore-sentinel-poll-qualification']
    modes += ['--apply-graph-csr-diagnostic', '--restore-graph-csr-diagnostic']
    modes += ['--apply-ffn-code-diagnostic', '--restore-ffn-code-diagnostic']
    modes += ['--apply-ffn-peer-lifetime-diagnostic', '--restore-ffn-peer-lifetime-diagnostic']
    modes += ['--apply-ffn-root-lifetime-diagnostic', '--restore-ffn-root-lifetime-diagnostic']
    modes += ['--apply-ffn-model-init', '--restore-ffn-model-init']
    modes += ['--apply-ffn-native-graph', '--restore-ffn-native-graph']
    modes += ['--apply-ffn-host-profile', '--restore-ffn-host-profile']
    modes += ['--apply-ffn-sealed-proof', '--restore-ffn-sealed-proof']
    modes += ['--apply-host-phase-diagnostic', '--restore-host-phase-diagnostic']
    modes += ['--apply-batch4-fetch-qualification', '--restore-batch4-fetch-qualification']
    modes += ['--apply-batch-raw-qualification', '--restore-batch-raw-qualification']
    modes += ['--apply-frontend-qualification', '--restore-frontend-qualification']
    # Failed native Q4 candidate is no longer installable. Known-clean
    # restore remains guarded;current cold/UNKNOWN can NEVER use it.
    modes += ['--restore-raw-unpack-cache-qualification']
    modes += ['--apply-tile-prefetch-qualification', '--restore-tile-prefetch-qualification']
    modes += ['--apply-command-reuse-qualification', '--restore-command-reuse-qualification']
    modes += ['--apply-peer-phase-qualification', '--restore-peer-phase-qualification']
    modes += ['--apply-q4-nibble-qualification', '--restore-q4-nibble-qualification']
    modes += ['--apply-q4-direct-qualification', '--restore-q4-direct-qualification']
    modes += ['--apply-local-q4-qualification', '--restore-local-q4-qualification']
    modes += ['--apply-embedding-validation', '--restore-embedding-validation']
    modes += ['--apply-rms-validation', '--restore-rms-validation']
    modes += ['--apply-projection-validation', '--restore-projection-validation']
    modes += ['--apply-decoder-validation', '--restore-decoder-validation']
    modes += ['--apply-gdn-input-validation', '--restore-gdn-input-validation']
    modes += ['--apply-gdn-recurrent-validation', '--restore-gdn-recurrent-validation']
    modes += ['--apply-gdn-rendezvous-validation', '--restore-gdn-rendezvous-validation']
    modes += ['--apply-conv-silu-precision-validation', '--restore-conv-silu-precision-validation']
    modes += ['--apply-resident-output-validation', '--restore-resident-output-validation']
    modes += ['--apply-resident-layer-validation', '--restore-resident-layer-validation']
    modes += ['--apply-mixed-live-validation', '--restore-mixed-live-validation']
    modes += ['--restore-recurrent-block-validation']
    modes += ['--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation']
    modes += ['--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation']
    modes += ['--apply-token-validation', '--restore-token-validation']
    modes += ['--apply-token-hold-recheck', '--restore-token-hold-recheck']
    modes += ['--apply-token-root-frontend', '--restore-token-root-frontend']
    modes += ['--apply-token-hidden-readout', '--restore-token-hidden-readout']
    modes += ['--apply-token-fault-readout', '--restore-token-fault-readout']
    modes += ['--apply-token-posted-cancel', '--restore-token-posted-cancel']
    modes += ['--apply-token-precise-beta', '--restore-token-precise-beta']
    modes += ['--apply-token-job-boot-fold', '--restore-token-job-boot-fold']
    modes += ['--apply-token-decode-benchmark', '--restore-token-decode-benchmark']
    modes += ['--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark']
    modes += list(RETAINED_MODES)
    modes += list(COMMIT_MODES)
    modes += list(COHORT_SOURCE_MODES)
    modes += list(PACKET_IDENTITY_MODES)
    modes += list(TID_BOUNDARY_MODES)
    modes += list(COUNTER_GATHER_MODES)
    modes += list(EMPTY_RECEIPT_POLL_MODES)
    modes += list(FLAT_MATRIX_MODES)
    modes += list(PREFETCH_MODES)
    modes += list(BATCHED_PREFETCH_MODES)
    modes += list(TWIN_BATCH_PREFETCH_MODES)
    parents += ['owner-token-hidden-validation']
    parents += ['owner-token-validation', 'owner-token-validation-underscore', 'owner-token-source-check']
    parents += ['owner-recurrent-prefix-validation', 'owner-recurrent-prefix-validation-underscore']
    parents += ['owner-recurrent-deadline-validation']
    parents += ['owner-embedding-validation']
    parents += ['owner-rms-validation']
    parents += ['owner-projection-validation']
    parents += ['owner-decoder-validation']
    parents += ['owner-gdn-input-validation', 'owner-gdn-input-validation-underscore']
    parents += ['owner-gdn-recurrent-validation', 'owner-gdn-recurrent-validation-underscore']
    parents += ['owner-gdn-rendezvous-validation', 'owner-gdn-rendezvous-validation-underscore']
    parents += ['owner-conv-precision-validation', 'owner-conv-precision-validation-underscore', 'owner-conv-precision-qualified']
    parents += ['owner-resident-output-validation', 'owner-resident-output-validation-underscore']
    parents += ['owner-resident-layer-validation', 'owner-resident-layer-validation-underscore']
    parents += ['owner-recurrent-block-validation', 'owner-recurrent-block-validation-underscore']
    for mode in modes:
        if mode in retired_modes:
            continue # Explicit hard refusal above,never an apply-positive fixture.
        if selected and mode not in selected:
            continue
        seen.add(mode)
        candidate = {'--apply-profile': ['candidatep'], '--apply-batch': ['candidateq'], '--apply-compact': ['candidatec'], '--apply-fast': ['candidatef'], '--apply-reuse': ['candidater'], '--restore-reuse': ['candidatef'], '--apply-cross-reuse': ['candidatex'], '--restore-cross-reuse': ['candidater'], '--apply-work': ['candidatew'], '--restore-work': ['candidatex'], '--apply-work-profile': ['candidatewp'], '--restore-work-profile': ['candidatew'], '--apply-work-posted': ['candidatews'], '--restore-work-posted': ['candidatew'], '--apply-work-cohort': ['candidatewc'], '--restore-work-cohort': ['candidatews'], '--apply-work-q4': ['candidatew4'], '--restore-work-q4': ['candidatewc'], '--apply-work-q4-inline': ['candidatew4i'], '--restore-work-q4-inline': ['candidatew4'], '--apply-work-span': ['candidatewsp'], '--restore-work-span': ['candidatew4i'], '--apply-work-fast-span': ['candidatewspf'], '--restore-work-fast-span': ['candidatewsp']}.get(mode, [])
        if mode == '--apply-work-span-code-reuse':
            candidate = ['candidatewspc']
        elif mode == '--restore-work-span-code-reuse':
            candidate = ['candidatewspf']
        elif mode == '--apply-work-int-sfpu':
            candidate = ['candidatewspi']
        elif mode == '--restore-work-int-sfpu':
            candidate = ['candidatewspc']
        elif mode == '--apply-work-pipelined':
            candidate = ['candidatewspq']
        elif mode == '--restore-work-pipelined':
            candidate = ['candidatewspi']
        elif mode == '--apply-work-static-vector':
            candidate = ['candidatewsps']
        elif mode == '--restore-work-static-vector':
            candidate = ['candidatewspq']
        elif mode == '--apply-work-cached-affine':
            candidate = ['candidatewspa']
        elif mode == '--restore-work-cached-affine':
            candidate = ['candidatewsps']
        elif mode == '--apply-work-replay-operand':
            candidate = ['candidatewspu']
        elif mode == '--restore-work-replay-operand':
            candidate = ['candidatewspa']
        elif mode == '--apply-work-dot':
            candidate = ['candidatewspd']
        elif mode == '--restore-work-dot':
            candidate = ['candidatewspu']
        elif mode == '--apply-work-output-dma':
            candidate = ['candidatewspo']
        elif mode == '--restore-work-output-dma':
            candidate = ['candidatewspd']
        elif mode == '--apply-local-delta-terminal':
            candidate = ['candidatewspg']
        elif mode == '--restore-local-delta-terminal':
            candidate = ['candidatewspv']
        elif mode == '--apply-local-delta':
            candidate = ['candidatewspv']
        elif mode == '--restore-local-delta':
            candidate = ['candidatewspj']
        elif mode == '--apply-work-queued-entry':
            candidate = ['candidatewspj']
        elif mode == '--restore-work-queued-entry':
            candidate = ['candidatewspo']
        elif mode == '--apply-current-profile':
            candidate = ['candidatecurrentprofile']
        elif mode == '--restore-current-profile':
            candidate = ['candidatewspg']
        elif mode == '--apply-sparse-profile' or mode == '--restore-sparse-export':
            candidate = ['candidate_sparse_profile']
        elif mode == '--restore-sparse-profile':
            candidate = ['candidatecurrentprofile']
        elif mode == '--apply-sparse-export':
            candidate = ['candidate_sparse_export']
        elif mode == '--apply-normal-clock':
            candidate = ['candidate_normal_clock']
        elif mode == '--restore-normal-clock':
            candidate = ['candidate_sparse_export']
        elif mode == '--apply-input-sum-cache':
            candidate = ['candidate_sum_cache']
        elif mode == '--restore-input-sum-cache':
            candidate = ['candidate_normal_clock']
        elif mode == '--apply-zero-minimum':
            candidate = ['candidate_zero_minimum']
        elif mode == '--restore-zero-minimum':
            candidate = ['candidate_sum_cache']
        elif mode == '--apply-native-operand':
            candidate = ['candidate_native_operand']
        elif mode == '--restore-native-operand':
            candidate = ['candidate_zero_minimum']
        elif mode == '--apply-compact-output':
            candidate = ['candidate_compact_output']
        elif mode == '--restore-compact-output':
            candidate = ['candidate_native_operand']
        elif mode == '--apply-compact-profile':
            candidate = ['candidate_compact_profile']
        elif mode == '--restore-compact-profile':
            candidate = ['candidate_compact_output']
        elif mode == '--apply-deferred-affine':
            candidate = ['candidate_deferred_affine']
        elif mode == '--restore-deferred-affine':
            candidate = ['candidate_compact_output']
        elif mode == '--apply-ordered-extents':
            candidate = ['candidate_ordered_extents']
        elif mode == '--restore-ordered-extents':
            candidate = ['candidate_compact_output']
        elif mode == '--apply-inline-config':
            candidate = ['candidate_inline_config']
        elif mode == '--restore-inline-config':
            candidate = ['candidate_deferred_affine']
        elif mode == '--apply-postscale-delta':
            candidate = ['candidate_postscale_delta']
        elif mode == '--restore-postscale-delta':
            candidate = ['candidate_inline_config']
        elif mode == '--apply-gdn-zero-diagnostic':
            candidate = ['candidate_gdn_zero_diagnostic']
        elif mode == '--restore-gdn-zero-diagnostic':
            candidate = ['candidate_postscale_delta']
        elif mode == '--apply-resident-root-diagnostic':
            candidate = ['candidate_resident_root_diagnostic']
        elif mode == '--restore-resident-root-diagnostic':
            candidate = ['candidate_gdn_zero_diagnostic']
        elif mode == '--apply-root-cycle-diagnostic':
            candidate = ['candidate_root_cycle_diagnostic']
        elif mode == '--restore-root-cycle-diagnostic':
            candidate = ['candidate_resident_root_diagnostic']
        elif mode == '--apply-block-affine-qualification':
            candidate = ['candidate_block_affine_qualification']
        elif mode == '--restore-block-affine-qualification':
            candidate = ['candidate_root_cycle_diagnostic']
        elif mode == '--apply-scale-reuse-qualification':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--restore-scale-reuse-qualification':
            candidate = ['candidate_block_affine_qualification']
        elif mode == '--apply-group32-qualification':
            candidate = ['candidate_group32_qualification']
        elif mode == '--restore-group32-qualification':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-native-q8-qualification':
            candidate = ['candidate_native_q8_qualification']
        elif mode == '--restore-native-q8-qualification':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-affine-rawdot-qualification':
            candidate = ['candidate_affine_rawdot_qualification']
        elif mode == '--restore-affine-rawdot-qualification':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-direct-srcb-qualification':
            candidate = ['candidate_direct_srcb_qualification']
        elif mode == '--restore-direct-srcb-qualification':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-sentinel-poll-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-graph-csr-diagnostic':
            candidate = ['candidate_graph_csr_diagnostic']
        elif mode == '--restore-graph-csr-diagnostic':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-code-diagnostic':
            candidate = ['candidate_ffn_code_diagnostic']
        elif mode == '--restore-ffn-code-diagnostic':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-peer-lifetime-diagnostic':
            candidate = ['candidate_ffn_peer_lifetime_diagnostic']
        elif mode == '--restore-ffn-peer-lifetime-diagnostic':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-root-lifetime-diagnostic':
            candidate = ['candidate_ffn_root_lifetime_diagnostic']
        elif mode == '--restore-ffn-root-lifetime-diagnostic':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-model-init':
            candidate = ['candidate_ffn_model_init']
        elif mode == '--restore-ffn-model-init':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-native-graph':
            candidate = ['candidate_ffn_native_graph']
        elif mode == '--restore-ffn-native-graph':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-host-profile':
            candidate = ['candidate_ffn_host_profile']
        elif mode == '--restore-ffn-host-profile':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-ffn-sealed-proof':
            candidate = ['candidate_ffn_sealed_proof']
        elif mode == '--restore-ffn-sealed-proof':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-host-phase-diagnostic':
            candidate = ['candidate_host_phase_diagnostic']
        elif mode == '--restore-host-phase-diagnostic':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-batch4-fetch-qualification':
            candidate = ['candidate_batch4_fetch_qualification']
        elif mode == '--restore-batch4-fetch-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-batch-raw-qualification':
            candidate = ['candidate_batch_raw_qualification']
        elif mode == '--restore-batch-raw-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-frontend-qualification':
            candidate = ['candidate_frontend_qualification']
        elif mode == '--restore-frontend-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-raw-unpack-cache-qualification':
            candidate = ['candidate_raw_unpack_cache_qualification']
        elif mode == '--restore-raw-unpack-cache-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-tile-prefetch-qualification':
            candidate = ['candidate_tile_prefetch_qualification']
        elif mode == '--restore-tile-prefetch-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-command-reuse-qualification':
            candidate = ['candidate_command_reuse_qualification']
        elif mode == '--restore-command-reuse-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-peer-phase-qualification':
            candidate = ['candidate_peer_phase_qualification']
        elif mode == '--restore-peer-phase-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-q4-nibble-qualification':
            candidate = ['candidate_q4_nibble_qualification']
        elif mode == '--restore-q4-nibble-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-q4-direct-qualification':
            candidate = ['candidate_q4_direct_qualification']
        elif mode == '--restore-q4-direct-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--apply-local-q4-qualification':
            candidate = ['candidate_local_q4_qualification']
        elif mode == '--restore-local-q4-qualification':
            candidate = ['candidate_sentinel_poll_qualification']
        elif mode == '--restore-sentinel-poll-qualification':
            candidate = ['candidate_direct_srcb_qualification']
        if mode == '--apply-embedding-validation':
            candidate = ['candidate_embedding_validation']
        elif mode == '--restore-embedding-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-rms-validation':
            candidate = ['candidate_rms_validation']
        elif mode == '--restore-rms-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-projection-validation':
            candidate = ['candidate_projection_validation']
        elif mode == '--restore-projection-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-decoder-validation':
            candidate = ['candidate_decoder_validation']
        elif mode == '--restore-decoder-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-gdn-input-validation':
            candidate = ['candidate_gdn_input_validation']
        elif mode == '--restore-gdn-input-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-gdn-recurrent-validation':
            candidate = ['candidate_gdn_recurrent_validation']
        elif mode == '--restore-gdn-recurrent-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-gdn-rendezvous-validation':
            candidate = ['candidate_gdn_rendezvous_validation']
        elif mode == '--restore-gdn-rendezvous-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-conv-silu-precision-validation':
            candidate = ['candidate_conv_silu_precision_validation']
        elif mode == '--restore-conv-silu-precision-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-resident-output-validation':
            candidate = ['candidate_resident_output_validation']
        elif mode == '--restore-resident-output-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-resident-layer-validation':
            candidate = ['candidate_resident_layer_validation']
        elif mode == '--restore-resident-layer-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-mixed-live-validation':
            candidate = ['candidate_mixed_live_validation']
        elif mode == '--restore-mixed-live-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-recurrent-block-validation':
            candidate = ['candidate_recurrent_block_validation']
        elif mode == '--restore-recurrent-block-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-recurrent-deadline-validation':
            candidate = ['candidate_recurrent_deadline_validation']
        elif mode == '--restore-recurrent-deadline-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-recurrent-prefix-validation':
            candidate = ['candidate_recurrent_prefix_validation']
        elif mode == '--restore-recurrent-prefix-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-validation':
            candidate = ['candidate_token_validation']
        elif mode == '--restore-token-validation':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-hold-recheck':
            candidate = ['candidate_token_hold_recheck']
        elif mode == '--restore-token-hold-recheck':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-root-frontend':
            candidate = ['candidate_token_root_frontend']
        elif mode == '--apply-token-hidden-readout':
            candidate = ['candidate_token_hidden_readout']
        elif mode == '--restore-token-hidden-readout':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-fault-readout':
            candidate = ['candidate_token_fault_readout']
        elif mode == '--restore-token-fault-readout':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-posted-cancel':
            candidate = ['candidate_token_posted_cancel']
        elif mode == '--restore-token-posted-cancel':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-precise-beta':
            candidate = ['candidate_token_precise_beta']
        elif mode == '--restore-token-precise-beta':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-job-boot-fold':
            candidate = ['candidate_token_job_boot_fold']
        elif mode == '--restore-token-job-boot-fold':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-decode-benchmark':
            candidate = ['candidate_token_decode_benchmark']
        elif mode == '--restore-token-decode-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-frontier-benchmark':
            candidate = ['candidate_token_frontier_benchmark']
        elif mode == '--restore-token-frontier-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-retained-cohort-benchmark':
            candidate = ['candidate_token_retained_cohort_benchmark']
        elif mode == '--restore-token-retained-cohort-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-commit-boundary-benchmark':
            candidate = ['candidate_token_commit_boundary_benchmark']
        elif mode == '--restore-token-commit-boundary-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-cohort-sources-benchmark':
            candidate = ['candidate_token_cohort_sources_benchmark']
        elif mode == '--restore-token-cohort-sources-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-packet-identity-benchmark':
            candidate = ['candidate_token_packet_identity_benchmark']
        elif mode == '--restore-token-packet-identity-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-tid-boundary-benchmark':
            candidate = ['candidate_token_tid_boundary_benchmark']
        elif mode == '--restore-token-tid-boundary-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-counter-gather-benchmark':
            candidate = ['candidate_token_counter_gather_benchmark']
        elif mode == '--restore-token-counter-gather-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-empty-receipt-poll-benchmark':
            candidate = ['candidate_token_empty_receipt_poll_benchmark']
        elif mode == '--restore-token-empty-receipt-poll-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-flat-matrix-benchmark':
            candidate = ['candidate_token_flat_matrix_benchmark']
        elif mode == '--restore-token-flat-matrix-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-prefetch-benchmark':
            candidate = ['candidate_token_prefetch_benchmark']
        elif mode == '--restore-token-prefetch-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-batched-prefetch-benchmark':
            candidate = ['candidate_token_batched_prefetch_benchmark']
        elif mode == '--restore-token-batched-prefetch-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--apply-token-twin-batch-prefetch-benchmark':
            candidate = ['candidate_token_twin_batch_prefetch_benchmark']
        elif mode == '--restore-token-twin-batch-prefetch-benchmark':
            candidate = ['candidate_scale_reuse_qualification']
        elif mode == '--restore-token-root-frontend':
            candidate = ['candidate_scale_reuse_qualification']
        candidate_links = [key + '-link' for key in candidate]
        previous_image = ['previous-debug44', 'previous-capture-debug44'] if mode in ('--apply-decoder-validation', '--restore-decoder-validation') else []
        if mode in ('--apply-conv-silu-precision-validation', '--restore-conv-silu-precision-validation'):
            previous_image.append('previous-debug52')
        if mode in ('--apply-resident-layer-validation', '--restore-resident-layer-validation'):
            previous_image.extend(('previous-layer69', 'previous-capture-layer69'))
        if mode in ('--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation'):
            previous_image.extend(('previous-debug71', 'old-tag71-metadata'))
        if mode in ('--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation'):
            previous_image.extend(('prefix-previous-debug71', 'prefix-previous-debug72', 'prefix-same-tag-wrong-bytes',
                                   'old-tag71-metadata', 'old-tag72-metadata'))
        if mode in TOKEN_MODES:
            previous_image.extend(('token-same-tag-wrong-bytes', 'token-other-revision', 'token-third-revision', 'old-tag71-metadata', 'old-tag72-metadata'))
        if mode in ('--apply-token-fault-readout','--restore-token-fault-readout'):
            previous_image.extend(('fault-old-tag75-bytes','fault-old-tag74-bytes'))
        if mode in ('--apply-token-posted-cancel', '--restore-token-posted-cancel'):
            previous_image.extend(('posted-old-fault', 'posted-old-hidden', 'posted-old-tag74', 'posted-same-tag-wrong'))
        if mode in ('--apply-token-precise-beta', '--restore-token-precise-beta'):
            previous_image.extend(('beta-old-posted', 'beta-old-fault', 'beta-old-hidden', 'beta-old-tag74', 'beta-same-tag-wrong'))
        if mode in ('--apply-token-job-boot-fold', '--restore-token-job-boot-fold'):
            previous_image.extend(('fold-old-beta', 'fold-old-posted', 'fold-old-fault', 'fold-old-hidden', 'fold-old-tag74', 'fold-same-tag-wrong'))
        if mode in ('--apply-token-decode-benchmark', '--restore-token-decode-benchmark'):
            previous_image.extend(('decode-old-fold', 'decode-old-beta', 'decode-old-posted', 'decode-old-fault', 'decode-old-hidden', 'decode-old-tag74', 'decode-same-tag-wrong'))
        if mode in ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark') + RETAINED_MODES + COMMIT_MODES + COHORT_SOURCE_MODES + PACKET_IDENTITY_MODES + TID_BOUNDARY_MODES + COUNTER_GATHER_MODES + EMPTY_RECEIPT_POLL_MODES + FLAT_MATRIX_MODES + PREFETCH_MODES:
            previous_image.extend(('frontier-old-decode', 'frontier-old-fold', 'frontier-old-beta', 'frontier-old-posted', 'frontier-old-fault', 'frontier-old-hidden', 'frontier-old-tag74', 'frontier-same-tag-wrong'))
        if mode in RETAINED_MODES:
            previous_image.append('retained-old-frontier')
            previous_image.append('retained-new-commit')
        if mode in COMMIT_MODES:
            previous_image.extend(('commit-old-frontier', 'commit-old-retained'))
        if mode in COHORT_SOURCE_MODES:
            previous_image.extend(('cohort-old-commit', 'cohort-old-retained', 'cohort-old-frontier'))
        if mode in PACKET_IDENTITY_MODES:
            previous_image.extend(('packet-old-cohort', 'packet-old-commit', 'packet-old-retained', 'packet-old-frontier'))
        if mode in TID_BOUNDARY_MODES:
            previous_image.extend(('tid-old-packet', 'tid-old-cohort', 'tid-old-commit', 'tid-old-retained', 'tid-old-frontier'))
        if mode in COUNTER_GATHER_MODES:
            previous_image.extend(('gather-old-tid', 'gather-old-packet', 'gather-old-cohort', 'gather-old-commit', 'gather-old-retained', 'gather-old-frontier'))
        if mode in EMPTY_RECEIPT_POLL_MODES:
            previous_image.extend(('empty-old-gather', 'empty-old-tid', 'empty-old-packet', 'empty-old-cohort', 'empty-old-commit', 'empty-old-retained', 'empty-old-frontier'))
        if mode in FLAT_MATRIX_MODES:
            previous_image.extend(('flat-old-empty','flat-old-gather','flat-old-tid','flat-old-packet','flat-old-cohort','flat-old-commit','flat-old-retained','flat-old-frontier'))
        if mode in PREFETCH_MODES:
            previous_image.extend('prefetch-old-'+name for name in ('flat','empty','gather','tid','packet','cohort','commit','retained','frontier'))
        if mode in BATCHED_PREFETCH_MODES:
            previous_image.extend('batched-old-' + name for name in ('prefetch', 'flat', 'empty', 'gather', 'tid', 'packet', 'cohort', 'commit', 'retained', 'frontier', 'same-tag-wrong'))
        if mode in BATCHED_PREFETCH_OLD_MODES:
            previous_image.append('old-route-new-batched')
        if mode in TWIN_BATCH_PREFETCH_MODES:
            previous_image.extend('twin-old-' + revision for revision in (*TWIN_BATCH_PREFETCH_OLD_REVISIONS, 'same-tag-wrong'))
        if mode in TWIN_BATCH_PREFETCH_OLD_MODES:
            previous_image.append('old-route-new-twin')
        if mode in FLAT_MATRIX_MODES + EMPTY_RECEIPT_POLL_MODES + COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark','--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-prefetch')
        if mode in EMPTY_RECEIPT_POLL_MODES + COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-flat')
        if mode in COUNTER_GATHER_MODES + TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-empty')
        if mode in TID_BOUNDARY_MODES + PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-gather')
        if mode in PACKET_IDENTITY_MODES + COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-tid')
        if mode in COHORT_SOURCE_MODES + COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-packet')
        if mode in COMMIT_MODES + RETAINED_MODES + ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark'):
            previous_image.append('old-route-new-cohort')
        hash_backup_mode = mode in ('--apply-decoder-validation', '--restore-decoder-validation', '--apply-gdn-input-validation', '--restore-gdn-input-validation', '--apply-gdn-recurrent-validation', '--restore-gdn-recurrent-validation', '--apply-gdn-rendezvous-validation', '--restore-gdn-rendezvous-validation', '--apply-conv-silu-precision-validation', '--restore-conv-silu-precision-validation', '--apply-resident-output-validation', '--restore-resident-output-validation', '--apply-resident-layer-validation', '--restore-resident-layer-validation')
        hash_backup_mode |= mode in ('--apply-mixed-live-validation', '--restore-mixed-live-validation')
        hash_backup_mode |= mode in ('--apply-recurrent-block-validation', '--restore-recurrent-block-validation')
        hash_backup_mode |= mode in ('--apply-recurrent-deadline-validation', '--restore-recurrent-deadline-validation')
        hash_backup_mode |= mode in ('--apply-recurrent-prefix-validation', '--restore-recurrent-prefix-validation')
        hash_backup_mode |= mode in ('--apply-token-validation', '--restore-token-validation')
        hash_backup_mode |= mode in ('--apply-token-hold-recheck', '--restore-token-hold-recheck')
        hash_backup_mode |= mode in ('--apply-token-root-frontend', '--restore-token-root-frontend')
        hash_backup_mode |= mode in ('--apply-token-hidden-readout', '--restore-token-hidden-readout')
        hash_backup_mode |= mode in ('--apply-token-fault-readout', '--restore-token-fault-readout')
        hash_backup_mode |= mode in ('--apply-token-posted-cancel', '--restore-token-posted-cancel')
        hash_backup_mode |= mode in ('--apply-token-precise-beta', '--restore-token-precise-beta')
        hash_backup_mode |= mode in ('--apply-token-job-boot-fold', '--restore-token-job-boot-fold')
        hash_backup_mode |= mode in ('--apply-token-decode-benchmark', '--restore-token-decode-benchmark')
        hash_backup_mode |= mode in ('--apply-token-frontier-benchmark', '--restore-token-frontier-benchmark')
        hash_backup_mode |= mode in RETAINED_MODES
        hash_backup_mode |= mode in COMMIT_MODES
        hash_backup_mode |= mode in COHORT_SOURCE_MODES
        hash_backup_mode |= mode in PACKET_IDENTITY_MODES
        hash_backup_mode |= mode in TID_BOUNDARY_MODES
        hash_backup_mode |= mode in COUNTER_GATHER_MODES
        hash_backup_mode |= mode in EMPTY_RECEIPT_POLL_MODES
        hash_backup_mode |= mode in FLAT_MATRIX_MODES
        hash_backup_mode |= mode in PREFETCH_MODES
        hash_backup_mode |= mode in BATCHED_PREFETCH_MODES
        hash_backup_mode |= mode in TWIN_BATCH_PREFETCH_MODES
        backup_links = ['backup-link'] if hash_backup_mode else []
        for fault in [None] + common + candidate + candidate_links + parents + previous_image + backup_links + (['loaded'] if mode == '--bootstrap' else hot):
            check(mode, fault)
            count += 1
        if hash_backup_mode:
            for existing_exact in (False, True):
                check(mode, legacy_backup=True, qualified_backup=existing_exact)
                count += 1
        if mode in TOKEN_MODES and mode.startswith('--restore-'):
            for existing_exact in (False, True):
                check(mode, legacy_backup=True, qualified_backup=existing_exact, other_token_backup=True)
                count += 1
    assert count and (not selected or seen == selected), 'Unknown or empty mode selection'
    print(f'PASS {count} actual updater filesystem/handshake/hash/symlink/owner/failure cases;NO native reload/reset or automatic rollback')
