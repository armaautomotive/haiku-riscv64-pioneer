#!/usr/bin/env python3
"""Actual private bank-END caller seal;CPU metadata ONLY,no native proof."""
import os
import pathlib
import re
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent
kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
source='\n'.join((kernel/name).read_text() for name in ('PioneerQ38MixedLiveNativeRole.h','PioneerQ38MixedLiveReadbackRole.h','PioneerQ38MixedLiveFinalReleaseRole.h'))
parents=set(re.findall(r'!defined\(([A-Z][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',source));assert len(parents)==23
defines={key:'1' for key in parents}
defines.update({key:'pioneer_mixed_end_call_test_'+key for key in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device')})
defines['PIONEER_DRIVER_BUILD_TAG']='0x51424c4e4f445256ULL'
defines['PIONEER_Q38_MIXED_LIVE_FINAL_END_CALL_TEST']='1'
with tempfile.TemporaryDirectory(prefix='pioneer-mixed-end-call-',dir='/private/tmp') as build:
 binary=pathlib.Path(build)/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  *['-D'+k+'='+v for k,v in sorted(defines.items())],str(here/'pioneer_q38_resident_layer_validation_end_call_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
