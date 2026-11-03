#!/usr/bin/env python3
"""Authenticated immutable cohort maps only; no native controller/device IO."""
import importlib.util
import json
import os
import pathlib
import subprocess
import sys

assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
image,out=map(pathlib.Path,sys.argv[1:])
assert image.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
generator=here.parent/'generate_dram_cohort_read_sweep_image.py'
spec=importlib.util.spec_from_file_location('cohort_image_generator',generator)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
expected=here.parents[2]/'src/add-ons/kernel/drivers/misc/BlackholeDramCohortReadSweepImage.h'
generated=out/'regenerated.h'
subprocess.run([sys.executable,str(generator),str(image/'program.elf'),str(generated)],check=True)
assert generated.read_bytes()==expected.read_bytes()==module.render(image/'program.elf').encode('ascii')
binary=out/'snapshot-data-test'
subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined',str(here/'pioneer_dram_cohort_snapshot_data_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary),str(image/'boot.bin'),str(image/'text.bin')],check=True)
data=(image/'program.elf').read_bytes();bad=[]
for i in range(32):
    changed=bytearray(data);changed[(i*509)%len(data)]^=1<<(i&7);bad.append(bytes(changed))
bad += [data[:n] for n in (0,1,4,51,len(data)//2,len(data)-1)]
histories=[]
for index,body in enumerate(bad):
    source=out/('corrupt-%02d.elf'%index);target=out/('MUST-NOT-EXIST-%02d.h'%index)
    source.write_bytes(body)
    r=subprocess.run([sys.executable,str(generator),str(source),str(target)],capture_output=True,text=True)
    assert r.returncode!=0 and not target.exists() and not r.stdout
    histories.append({'case':index,'exit':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
(out/'refusal-histories.json').write_text(json.dumps(histories,indent=2)+'\n')
print('PASS_COHORT_IMMUTABLE_GENERATOR wholeELFAndSections1 headerExact1 corruptedOrTruncatedRefusals38 beforeOutput1;'
      'NOT native ownership/driver/install/physical/model/Work permission',flush=True)
