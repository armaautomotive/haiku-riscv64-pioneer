#!/usr/bin/env python3
"""Verbatim public routing/admission over handler stubs,NOT native/card IO."""
import os
import pathlib
import subprocess
import tempfile
here=pathlib.Path(__file__).resolve().parent;kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
source=(kernel/'tenstorrent_pci_probe.cpp').read_text()
start=source.index('static status_t\nProbeControl(');branch=source.index('#ifdef PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT',start)
head=source[start:branch];assert 'geteuid() != 0' in head and 'PioneerQ38MixedLiveNativeRouting.inc' in head
lines=source[branch:].splitlines(keepends=True);depth=0;selected=[]
for line in lines:
 text=line.lstrip()
 if text.startswith(('#if ','#if\t','#ifdef ','#ifndef ')):depth+=1
 selected.append(line)
 if text.startswith('#endif'):
  depth-=1
  if not depth:break
assert depth==0 and 'Q38RecurrentPrefixValidationControl(instance,operation,data,length)' in ''.join(selected)
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-routing-',dir='/private/tmp') as temporary:
 out=pathlib.Path(temporary);(out/'prefix_actual_routing.inc').write_text(head+''.join(selected)+'\n}\n')
 for physical in (0,1):
  binary=out/('test'+str(physical))
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
   '-DPREFIX_ROUTING_PHYSICAL='+str(physical),'-I'+str(out),'-I'+str(kernel),str(here/'pioneer_q38_recurrent_prefix_validation_routing_test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
