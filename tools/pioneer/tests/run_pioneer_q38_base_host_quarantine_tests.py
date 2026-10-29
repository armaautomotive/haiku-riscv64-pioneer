#!/usr/bin/env python3
"""Extract and execute the production metadata-only quarantine method."""
import os
import pathlib
import subprocess
import tempfile

assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
host=source[source.index('struct PioneerHostPageIO {'):source.index('struct PioneerOutputBufferIO {')]
signature='\tvoid QuarantineHostPage() {'
assert host.count(signature)==1
begin=host.index(signature);at=host.index('{',begin);depth,end=1,at+1
while depth:
    depth+=(host[end]=='{')-(host[end]=='}');end+=1
with tempfile.TemporaryDirectory(prefix='pioneer-base-quarantine-',dir='/private/tmp') as tmp:
    base=pathlib.Path(tmp)
    (base/'token_native_base_host_quarantine.inc').write_text(host[begin:end]+'\n')
    binary=base/'test'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-I'+str(base),str(here/'pioneer_q38_base_host_quarantine_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
