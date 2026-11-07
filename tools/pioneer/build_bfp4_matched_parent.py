#!/usr/bin/env python3
"""Generate a fixed matched121 one-shot parent from the pinned legacy parent.

BUILD ONLY: no remote access, driver installation, GPU access or authority to
run the resulting script. The output must pass simulated fault tests first.
"""
import hashlib
from pathlib import Path
import sys

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('use NEW_ABSOLUTE_OUTPUT_FILE')
output = Path(sys.argv[1])
if not output.is_absolute() or output.exists() or output.is_symlink():
    raise SystemExit('output must be a new absolute file')
template = Path(__file__).with_name('pioneer_native_bfp4_dram_physical_120.sh')
raw = template.read_bytes()
if hashlib.sha256(raw).hexdigest() != 'd686c03b2e9a8346937e0afc0e58e75aa6e4e7dfd2c295647e0542fca1e32741':
    raise SystemExit('legacy parent changed; re-audit before generation')
source = raw.decode('utf-8')
def replace_once(old, new):
    global source
    if source.count(old) != 1:
        raise ValueError('template transformation is not unique: ' + old)
    source = source.replace(old, new, 1)
replace_once('# ONE fixed real-weight timed BFP4 GDDR K5120 trial.',
             '# ONE fixed matched-input timed BFP4 GDDR K5120 trial, diagnostic121.')
replace_once('stage=/boot/home/pioneer-native-bfp4-dram120-20261106-a',
             'stage=/boot/home/pioneer-native-bfp4-matched121-20261107-a')
replace_once('candidate=$stage/tenstorrent_pci_probe-native-bfp4-dram120',
             'candidate=$stage/tenstorrent_pci_probe-native-bfp4-matched121')
replace_once('candidate_hash=98e28838a606fd00f32cdee8b027894ad9e7e3a8a49021c5371d9eb44aff239f',
             'candidate_hash=ad9bffb2ca52de673e69a2a07e395cf86b8dbe0d8f9f32157fa40bcff9187325')
replace_once('client_hash=369d3bd742c59303a9c19fcb1886bb673844bb6656693a49b6be5ea65e766f57',
             'client_hash=c7ab8febf490b4925a81b6e3a067a7af08db47cc935d37db3397e6e67ff094e9')
replace_once('idle && metadata 2026102100010012 16 && health || exit 1',
             'idle && metadata 2026102100010012 18 && health || exit 1')
replace_once('exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110600010120 16 17 "$candidate" || exit 1',
             'exchange "$stable_hash" "$candidate_hash" 2026102100010012 2026110700010121 18 19 "$candidate" || exit 1')
replace_once('idle && metadata 2026110600010120 17 && exact',
             'idle && metadata 2026110700010121 19 && exact')
replace_once('awk -v generation=17 -f "$gate"', 'awk -v generation=19 -f "$gate"')
replace_once('metadata 2026110600010120 17 || exit 1', 'metadata 2026110700010121 19 || exit 1')
replace_once('exchange "$candidate_hash" "$stable_hash" 2026110600010120 2026102100010012 17 18 "$stable" || exit 1',
             'exchange "$candidate_hash" "$stable_hash" 2026110700010121 2026102100010012 19 20 "$stable" || exit 1')
replace_once('exact "$installed" "$stable_hash" && metadata 2026102100010012 18 && idle && health || exit 1',
             'exact "$installed" "$stable_hash" && metadata 2026102100010012 20 && idle && health || exit 1')
replace_once('STABLE041_RESTORED_BFP4_GDDR_BANDS_EXACT192_PASSED',
             'STABLE041_RESTORED_MATCHED_BFP4_GDDR_EXACT192_PASSED')
replace_once('BFP4_GDDR_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM',
             'MATCHED_BFP4_GDDR_KNOWN_CLEAN_NO_MODEL_SPEED_CLAIM')
with output.open('x', encoding='utf-8', newline='\n') as file:
    file.write(source)
output.chmod(0o700)
print(hashlib.sha256(source.encode()).hexdigest(), str(output))
print('BUILD ONLY: do not install or run without separate safety qualification')
