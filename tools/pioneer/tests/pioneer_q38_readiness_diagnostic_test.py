#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[3]
worker = (root / 'tools/pioneer/pioneer_assistant_qwen27_full_offload.cpp').read_text()
prepared = (root / 'tools/pioneer/PioneerQ38PreparedQ4SessionSource.h').read_text()
cache = (root / 'tools/pioneer/PioneerQ38NativeQ4WeightCache.h').read_text()
builder = (root / 'tools/pioneer/build_assistant_qwen27_full_offload.py').read_text()
client = (root / 'tools/pioneer/pioneer_q38_full_offload_ready_only_client.cpp').read_text()
session = (root / 'tools/pioneer/PioneerQ38FullOffloadSession.h').read_text()

diagnostic = worker[worker.index('static int ReadinessDiagnostic()'):worker.index('int main(int argc,char** argv){')]
for phase in ('phase=vocabulary', 'phase=prepared_q4_preflight', 'phase=metadata_probe_only'):
    assert phase in diagnostic
assert diagnostic.index('runtime.LoadVocabulary()') < diagnostic.index('runtime.session->Preflight()')
assert diagnostic.index('runtime.session->Preflight()') < diagnostic.index('runtime.session->Probe()')
assert 'runtime.session->Begin(' not in diagnostic
assert 'runtime.Generate(' not in diagnostic
assert 'NO_GENERATE_SENT' in diagnostic and 'NO_CPU_INFERENCE' in diagnostic
assert 'A::Generate' not in client and 'A::Load' in client and 'A::Unload' in client
assert 'PIONEER_Q38_READINESS_DIAGNOSTIC' in prepared
assert "readiness_diagnostic='--readiness-diagnostic' in args" in builder
assert "-DPIONEER_Q38_READINESS_DIAGNOSTIC=1" in builder
assert "prepared_q4 and args.count('--readiness-diagnostic')==1" in builder
assert "gdn_l1_tag206='--gdn-l1-tag206' in args" in builder
assert "-DPIONEER_Q38_EXPECTED_DRIVER_TAG=0x2026111500010206ULL" in builder
assert 'if gdn_l1_tag206:smoke_command.insert(1,' in builder
assert '/boot/home/pioneer-q38-readiness-tag206-20261116-b/readiness-diagnostic-tag206' in builder
assert 'r.build_tag!=expectedDriverTag' in session
assert 'PIONEER_Q38_EXPECTED_DRIVER_TAG != 0x2026111500010206ULL' in session
assert 'READINESS_Q4_CACHE_SOURCE_PART %u/%zu' in prepared
assert 'cache.OpenExistingDirectory(directory,error)' in cache
assert 'cache.OpenExisting(e.manifestKey' in cache
assert 'cache.OpenExisting(e.bundleKey' in cache
assert 'prepared_q4 and args.count(\'--readiness-diagnostic\')==1' in builder
sys.path.insert(0, str(root / 'tools/pioneer'))
from pioneer_native_q4_cache_recipe import recipe
source_fingerprint, cache_identity = recipe(root)
assert source_fingerprint == 'bc4adf382e35955f598e599f189819b0c2f3ede6acfc26ed756ddb1a7ad688c4'
assert cache_identity == '70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6'
print('PASS readiness diagnostic is ordered vocabulary -> prepared cache -> metadata probe; no Generate/CPU inference')
