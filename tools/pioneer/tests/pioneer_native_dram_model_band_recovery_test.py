#!/usr/bin/env python3
"""Actual fresh81 recovery flow;local files/fake admin only,NO device."""
import pathlib
import subprocess
import tempfile

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
source = (here / 'pioneer_native_dram_model_band_recovery_restore.sh').read_text()
assert 'idle && metadata 2026110400010081 1 || exit 1' in source
assert 'UNPROVEN_GDDR_BANDS_NO_END_RESET_RETRY_RESTORE' in source and '--fixed-gddr-learned-bands' not in source
flow = source.split('# NATIVE_GDDR_FRESH_RESTORE_FLOW_BEGIN\n', 1)[1].split('# NATIVE_GDDR_FRESH_RESTORE_FLOW_END', 1)[0]
folder = pathlib.Path(tempfile.mkdtemp(prefix='pioneer-gddr-recovery.'))
program = folder / 'flow.sh'
program.write_text('''set -eu
stage=$1;failure=$2;stable=$stage/stable;installed=$stage/installed;stable_hash=STABLE;candidate_hash=CANDIDATE;admin=Admin
step(){ n=$(cat "$stage/count");n=$((n+1));echo "$n" > "$stage/count";echo "$1" >> "$stage/events";[ "$n" != "$failure" ]; }
exact(){ step exact; };idle(){ step idle; };metadata(){ step "metadata:$1:$2"; };Admin(){ step "admin:$1:$2"; }
mkdir(){ step mkdir && command mkdir "$@"; };printf(){ step printf && command printf "$@"; }
cp(){ step cp && command cp "$@"; };chmod(){ step chmod && command chmod "$@"; }
mv(){ step mv && command mv "$@"; };sync(){ step sync; };echo(){ command echo "$@"; }
''' + flow)
count = None
for failure in range(100):
    stage = folder / str(failure);stage.mkdir()
    (stage / 'count').write_text('0\n');(stage / 'stable').write_text('STABLE');(stage / 'installed').write_text('CANDIDATE')
    result = subprocess.run(['sh', str(program), str(stage), str(failure)], text=True, capture_output=True)
    events = (stage / 'events').read_text().splitlines()
    if not failure:
        assert result.returncode == 0 and (stage / 'installed').read_text() == 'STABLE'
        assert events.count('admin:--quiesce:2026110400010081') == 1 and events.count('admin:--wait:2026102100010012') == 1
        assert (stage / 'recovery.state').read_text().strip() == 'NEW_POWER_EPOCH_STABLE041_RESTORED_CACHED_ASSISTANT_PRESERVED'
        count = len(events)
    else:
        assert result.returncode != 0 and len(events) == failure, (failure, events, result.stdout, result.stderr)
    if failure == count:
        break
assert count and count < 100
metadata = source.split('metadata(){\n', 1)[1].split('\n}\n', 1)[0]
meta_program = folder / 'metadata.sh'
meta_program.write_text('set -eu\nadmin=Admin;value_text=$1\nAdmin(){ command printf "%s\\n" "$value_text"; }\nmetadata(){\n' + metadata + '\n}\nmetadata 2026110400010081 1\n')
normal = 'P100A_RELOAD driver55 tag2026110400010081 generation1 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
variants = [normal, normal.replace('generation1', 'generation11'), normal.replace('cold0', 'cold1'),
            normal.replace('active_owner0', 'active_owner1'), normal.replace('quiesced0', 'quiesced1'),
            normal.replace('open_count1', 'open_count2'), normal.replace('guard1;', 'guard0;'), normal.replace('0081', '0080')]
for i, value in enumerate(variants):
    result = subprocess.run(['sh', str(meta_program), value], text=True, capture_output=True)
    assert (result.returncode == 0) == (i == 0)
print(f'PASS actual fresh81 recovery:{count + 1} local first-failure flow+{len(variants)} epoch/owner/cold metadata cases;single swap only after fresh-epoch gate,NO GPU/test/END/reset/automatic old-lease cleanup authority')
print('CPU-only recovery evidence:', folder)
