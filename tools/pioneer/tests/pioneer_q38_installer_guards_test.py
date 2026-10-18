#!/usr/bin/env python3
"""Run ACTUAL installers under /bin/sh in private fake filesystems;NO device IO."""
import hashlib
import pathlib
import re
import shlex
import shutil
import subprocess
import tempfile

DIRECTORY = pathlib.Path(__file__).resolve().parent.parent


def assignments(text):
    values = {}
    for line in text.splitlines():
        match = re.fullmatch(r'([a-z][a-z0-9_]*)=([^;() ]+)', line)
        if match:
            name, value = match.groups()
            values[name] = re.sub(r'\$([a-z][a-z0-9_]*)', lambda m: values.get(m[1], m[0]), value)
    return values


def check(script, fault=None, restore=None, text_override=None):
    text = script.read_text() if text_override is None else text_override
    original = assignments(text)
    pins = {v for v in original.values() if re.fullmatch('[0-9a-f]{64}', v)}
    payloads = {pin: ('fixture-' + pin).encode() for pin in pins}
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-installer-') as temporary:
        root = pathlib.Path(temporary)
        def path(value):
            assert value.startswith('/boot/home/')
            return root / value.lstrip('/')
        def put(value, data):
            target = path(value)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            return target
        old = payloads[original['old']]
        new = payloads[original['new']]
        q43 = payloads[original.get('qualified43', original.get('qualified'))]
        q52 = payloads[original.get('qualified52', original['old'])]
        q54 = payloads[original.get('qualified54', original['old'])]
        q55 = payloads[original.get('qualified55', original['old'])]
        path(original['stage']).mkdir(parents=True)
        installed = put(original['installed'], old)
        for key, pin in [('candidate', 'new'), ('client', 'clientsha'), ('inference', 'inferencesha')]:
            if key in original:
                put(original[key], payloads[original[pin]])
        if 'source43' in original:
            put(original['source43'], q43)
        if 'prior' in original:
            for name in ['qualified43-backup', 'qualified43-9e098d02-backup']:
                put(original['prior'] + '/' + name, q43)
            for name in ['qualified52-backup', 'qualified52-1de41744-backup']:
                put(original['prior'] + '/' + name, q52)
            if 'qualified54' in original:
                put(original['prior'] + '/qualified54-frontend-backup', q54)
            if 'qualified55' in original:
                put(original['prior'] + '/prior55-vector-backup', q55)
        device = path(original['device'])
        device.parent.mkdir(parents=True)
        device.symlink_to('../../bin/tenstorrent_pci_probe')
        if fault in ['candidate', 'client', 'inference', 'installed']:
            path(original[fault]).write_bytes(b'')
        elif fault == 'candidate-symlink':
            candidate = path(original['candidate'])
            other = candidate.with_name(candidate.name + '.real')
            candidate.rename(other)
            candidate.symlink_to(other.name)
        elif fault == 'stage-symlink':
            stage = path(original['stage'])
            other = stage.with_name(stage.name + '.real')
            stage.rename(other)
            stage.symlink_to(other.name)
        elif fault == 'backup-exists':
            put(original['backup43'], b'preexisting user backup')
        elif fault == 'incoming-exists':
            put(original['incoming'], b'preexisting incoming')
        elif fault == 'device-link':
            device.unlink()
            device.symlink_to('../../bin/wrong-device')
        before = installed.read_bytes()
        images = ['KERNEL TEAM:', '1 0x0 0x0 0 0 kernel_riscv64']
        images += [f'{i} 0x0 0x0 0 0 module{i}' for i in range(2, 52)]
        if fault == 'loaded':
            images.append('999 0x0 0x0 0 0 tenstorrent_pci_probe')
        teams = ['Id Team']
        if fault == 'owner':
            teams.append('999 /boot/home/pioneer-qwen38-dispatch-20261018-f/pioneer-q38-client')
        if fault == 'owner-truncated':
            teams.append('999 /boot/home/pioneer-qwen38-dispatch-20261018-f/pioneer-q')
        if fault == 'owner-relative-inference':
            teams.append('999 ./inference-pool-frontend-f2096ee7')
        if fault == 'owner-absolute-inference':
            teams.append('999 /boot/home/config/non-packaged/bin/inference-q38-public')
        if fault == 'listimage-too-few':
            images = images[:20]
        if fault == 'listimage-missing-kernel':
            images[1] = images[1].replace('kernel_riscv64', 'not-the-kernel')
        image_body = 'printf "%s\\n" ' + ' '.join(map(shlex.quote, images)) + ';'
        team_body = 'printf "%s\\n" ' + ' '.join(map(shlex.quote, teams)) + ';'
        def late_body(name, body, extra):
            counter = shlex.quote(str(root / (name + '-count')))
            return ('n=0; if test -f ' + counter + '; then read -r n < ' + counter + '; fi; '
                    'n=$((n+1)); printf "%s\\n" "$n" > ' + counter + '; ' + body +
                    ' if test "$n" -ge 2; then printf "%s\\n" ' + shlex.quote(extra) + '; fi; return 0;')
        if fault == 'loaded-late':
            image_body = late_body('images', image_body, '999 0x0 0x0 0 0 tenstorrent_pci_probe')
        if fault == 'owner-late':
            team_body = late_body('teams', team_body, '999 ./inference-vector-62fef745')
        if fault == 'listimage-error':
            image_body = 'return 1;'
        if fault == 'listimage-empty':
            image_body = 'return 0;'
        mocks = '\n'.join([
            'uname() { printf "%s\\n" ' + ('Linux' if fault == 'platform' else 'Haiku') + '; }',
            'listimage() { ' + image_body + ' }',
            'ps() { ' + team_body + ' }',
            'sha256sum() { ' + shlex.quote(shutil.which('shasum')) + ' -a 256 "$@"; }',
            'sync() { :; }',
        ])
        rewritten = text.replace('/boot/home', str(root / 'boot/home'))
        for pin, payload in payloads.items():
            rewritten = rewritten.replace(pin, hashlib.sha256(payload).hexdigest())
        local = root / 'isolated-installer.sh'
        local.write_text(mocks + '\n' + rewritten)
        result = subprocess.run(['/bin/sh', str(local), '--apply'], capture_output=True, text=True, timeout=15)
        if fault:
            assert result.returncode != 0 and installed.read_bytes() == before, (
                script.name, fault, result.returncode, result.stdout, result.stderr, 'INSTALLER_CHANGED_DESPITE_GUARD_FAILURE')
        else:
            assert result.returncode == 0 and installed.read_bytes() == new, (script.name, result.stdout, result.stderr)
            if restore:
                result = subprocess.run(['/bin/sh', str(local), restore], capture_output=True, text=True, timeout=15)
                expected = old if restore == '--restore57' else q55 if restore == '--restore55' else q54 if restore == '--restore54' else q43 if restore == '--restore43' else q52
                assert result.returncode == 0 and installed.read_bytes() == expected, (script.name, restore, result.stdout, result.stderr)


if __name__ == '__main__':
    scripts = sorted(DIRECTORY.glob('pioneer_q38_dispatch*switch_unloaded.sh'))
    assert len(scripts) >= 5
    faults = ['candidate', 'client', 'inference', 'installed', 'candidate-symlink', 'stage-symlink',
              'backup-exists', 'incoming-exists', 'device-link', 'loaded', 'owner', 'owner-truncated',
              'owner-relative-inference', 'owner-absolute-inference', 'platform',
              'loaded-late', 'owner-late', 'listimage-error', 'listimage-empty',
              'listimage-too-few', 'listimage-missing-kernel']
    count = 0
    for script in scripts:
        for fault in faults:
            if fault == 'inference' and 'inference' not in assignments(script.read_text()):
                continue  # diagnostic57 deliberately has NO model binary/capability
            check(script, fault=fault)
            count += 1
        restores = [None, '--restore43', '--restore52']
        if '--restore54' in script.read_text():
            restores.append('--restore54')
        if '--restore55' in script.read_text():
            restores.append('--restore55')
        if '--restore57' in script.read_text():
            restores.append('--restore57')
        for restore in restores:
            check(script, restore=restore)
            count += 1
    # Demonstrate that this harness rejects the ACTUAL failure mode,not
    # merely fixture mistakes:remove only the artifact-chain exit guard.
    reference = DIRECTORY / 'pioneer_q38_dispatch_physical_audit_switch_unloaded.sh'
    vulnerable = reference.read_text()
    chain = next(line for line in vulnerable.splitlines() if line.startswith('exact "$candidate"') and '"$client"' in line)
    assert chain.endswith(' || exit 1')
    vulnerable = vulnerable.replace(chain, chain.removesuffix(' || exit 1'))
    try:
        check(reference, fault='client', text_override=vulnerable)
    except AssertionError as failure:
        assert 'INSTALLER_CHANGED_DESPITE_GUARD_FAILURE' in str(failure)
        print('EXPECTED regression rejection:vulnerable set-e/AND chain installs despite empty client;private fixture ONLY')
    else:
        raise AssertionError('guard regression harness did not reject vulnerable baseline')
    print(f'PASS {count} ACTUAL shell installer guard/restore cases:bad or empty artifact,symlink,existing backup/incoming,loaded module,full/truncated owner,platform refuse without installed writes;NO native/device IO')
