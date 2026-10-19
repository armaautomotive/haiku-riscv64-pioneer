#!/usr/bin/env python3
"""Actual /bin/sh updater in disposable fake Haiku filesystems;NO device IO."""
import hashlib
import pathlib
import shlex
import shutil
import subprocess
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
        payloads = {key: ('test-' + values[key]).encode() for key in ['original', 'hasha', 'hashb', 'hashp', 'hashq', 'hashc', 'hashf', 'hashr', 'hashx', 'hashw', 'hashwp', 'hashws', 'hashwc', 'hashw4', 'hashw4i', 'hashwsp', 'hashwspf', 'hashwspc', 'hashwspi', 'guardsha']}
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
    common = ['candidatea', 'candidateb', 'guardsource', 'client', 'installed', 'candidatea-link', 'client-link',
              'installed-link', 'device-link', 'stage-link', 'incoming', 'guardincoming', 'backup', 'platform']
    hot = ['guard', 'guard-link', 'cold', 'active', 'open', 'tag', 'generation', 'quiesced', 'guard-version',
           'duplicate-status', 'empty-status', 'status-error', 'quiesce-error', 'wait-error']
    for mode in ['--bootstrap', '--apply-a', '--apply-b', '--apply-profile', '--restore-profile', '--apply-batch', '--restore-batch', '--apply-compact', '--restore-compact', '--apply-fast', '--restore-fast', '--apply-reuse', '--restore-reuse', '--apply-cross-reuse', '--restore-cross-reuse', '--apply-work', '--restore-work', '--apply-work-profile', '--restore-work-profile', '--apply-work-posted', '--restore-work-posted', '--apply-work-cohort', '--restore-work-cohort', '--apply-work-q4', '--restore-work-q4', '--apply-work-q4-inline', '--restore-work-q4-inline'] + ['--apply-work-span', '--restore-work-span', '--apply-work-fast-span', '--restore-work-fast-span', '--apply-work-span-code-reuse', '--restore-work-span-code-reuse', '--apply-work-int-sfpu', '--restore-work-int-sfpu']:
        candidate = {'--apply-profile': ['candidatep'], '--apply-batch': ['candidateq'], '--apply-compact': ['candidatec'], '--apply-fast': ['candidatef'], '--apply-reuse': ['candidater'], '--restore-reuse': ['candidatef'], '--apply-cross-reuse': ['candidatex'], '--restore-cross-reuse': ['candidater'], '--apply-work': ['candidatew'], '--restore-work': ['candidatex'], '--apply-work-profile': ['candidatewp'], '--restore-work-profile': ['candidatew'], '--apply-work-posted': ['candidatews'], '--restore-work-posted': ['candidatew'], '--apply-work-cohort': ['candidatewc'], '--restore-work-cohort': ['candidatews'], '--apply-work-q4': ['candidatew4'], '--restore-work-q4': ['candidatewc'], '--apply-work-q4-inline': ['candidatew4i'], '--restore-work-q4-inline': ['candidatew4'], '--apply-work-span': ['candidatewsp'], '--restore-work-span': ['candidatew4i'], '--apply-work-fast-span': ['candidatewspf'], '--restore-work-fast-span': ['candidatewsp']}.get(mode, [])
        if mode == '--apply-work-span-code-reuse':
            candidate = ['candidatewspc']
        elif mode == '--restore-work-span-code-reuse':
            candidate = ['candidatewspf']
        elif mode == '--apply-work-int-sfpu':
            candidate = ['candidatewspi']
        elif mode == '--restore-work-int-sfpu':
            candidate = ['candidatewspc']
        for fault in [None] + common + candidate + (['loaded', 'owner'] if mode == '--bootstrap' else hot):
            check(mode, fault)
            count += 1
    print(f'PASS {count} actual updater filesystem/handshake/hash/symlink/owner/failure cases;NO native reload/reset or automatic rollback')
