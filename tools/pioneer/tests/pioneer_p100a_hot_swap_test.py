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


def check(mode, fault=None):
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
        put('guardsource', payloads['guardsha'])
        if mode != '--bootstrap':
            put('guard', payloads['guardsha'])
        device = path(values['device'])
        device.parent.mkdir(parents=True, exist_ok=True)
        device.symlink_to('../../bin/tenstorrent_pci_probe')
        oldtag = values[oldtagkey]
        status = f'P100A_RELOAD driver55 tag{oldtag} generation3 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
        if fault == 'cold':
            status = status.replace('cold0', 'cold1')
        if fault == 'active':
            status = status.replace('active_owner0', 'active_owner1')
        if fault == 'open':
            status = status.replace('open_count1', 'open_count2')
        if fault == 'tag':
            status = status.replace(oldtag, '0123456789abcdef')
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
            ' --quiesce) exit ' + ('1' if fault == 'quiesce-error' else '0') + ';;\n'
            ' --wait) test "$3" = 0000000000000003; exit ' + ('1' if fault == 'wait-error' else '0') + ';;\n'
            ' *) exit 1;;\nesac\n').encode()
        client = put('client', client_body)
        client.chmod(0o700)
        if fault in values and (fault.startswith('candidate') or fault in ['guardsource', 'client', 'installed', 'guard']):
            path(values[fault]).write_bytes(b'wrong')
        if fault in ['candidatea-link', 'client-link', 'guard-link', 'installed-link']:
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
        if fault == 'backup':
            target = path(values['stage']) / ('qualified55-before-hot-reload' if mode == '--bootstrap' else 'qualified-reload-' + oldtag)
            target.write_bytes(b'preexisting backup')
        before = installed.read_bytes()
        images = ['KERNEL TEAM:', '1 0x0 0x0 0 0 kernel_riscv64'] + [f'{i} 0x0 0x0 0 0 module{i}' for i in range(2, 52)]
        if fault == 'loaded':
            images.append('999 0x0 0x0 0 0 tenstorrent_pci_probe')
        teams = ['Id Team'] + (['999 ./inference-vector-21e18126'] if fault == 'owner' else [])
        mocks = '\n'.join([
            'uname() { printf "%s\\n" ' + ('Linux' if fault == 'platform' else 'Haiku') + '; }',
            'listimage() { printf "%s\\n" ' + ' '.join(map(shlex.quote, images)) + '; }',
            'ps() { printf "%s\\n" ' + ' '.join(map(shlex.quote, teams)) + '; }',
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
        if fault:
            assert result.returncode != 0, (mode, fault, result.stdout, result.stderr)
            if fault != 'wait-error':
                assert installed.read_bytes() == before, (mode, fault, 'REPLACED_DESPITE_GUARD_FAILURE')
            else:
                assert installed.read_bytes() == payloads[ending]
                assert 'NO_AUTOMATIC_ROLLBACK_OR_RESET' in result.stdout
        else:
            assert result.returncode == 0, (mode, result.stdout, result.stderr)
            assert installed.read_bytes() == payloads[ending]
            if mode != '--bootstrap':
                assert (root / 'calls').read_text().splitlines() == ['--status', '--quiesce', '--wait']


if __name__ == '__main__':
    count = 0
    selected = set(sys.argv[1:])
    seen = set()
    common = ['candidatea', 'candidateb', 'guardsource', 'client', 'installed', 'candidatea-link', 'client-link',
              'installed-link', 'device-link', 'stage-link', 'incoming', 'guardincoming', 'backup', 'platform']
    hot = ['guard', 'guard-link', 'cold', 'active', 'open', 'tag', 'generation', 'quiesced', 'guard-version',
           'duplicate-status', 'empty-status', 'status-error', 'quiesce-error', 'wait-error']
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
    for mode in modes:
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
        for fault in [None] + common + candidate + (['loaded', 'owner'] if mode == '--bootstrap' else hot):
            check(mode, fault)
            count += 1
    assert count and (not selected or seen == selected), 'Unknown or empty mode selection'
    print(f'PASS {count} actual updater filesystem/handshake/hash/symlink/owner/failure cases;NO native reload/reset or automatic rollback')
