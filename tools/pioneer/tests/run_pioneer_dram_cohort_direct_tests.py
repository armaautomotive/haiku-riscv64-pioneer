#!/usr/bin/env python3
"""Actual native IO old/new syscall counts plus whole public/client regressions."""
import os
import pathlib
import subprocess
import sys
assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent
base=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
for name,flags in (('reference',['-DCOHORT_SCOPE_REFERENCE_TEST=1']),('direct',[])):
    binary=out/(name+'-endpoints')
    subprocess.run([*base,*flags,str(here/'pioneer_dram_cohort_direct_test.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
binary=out/'direct-client'
subprocess.run([*base,'-DCOHORT_DIRECT_ENDPOINT_TEST=1','-I'+str(here/'cli-stubs'),
 str(here/'pioneer_dram_cohort_client_test.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary)],check=True)
