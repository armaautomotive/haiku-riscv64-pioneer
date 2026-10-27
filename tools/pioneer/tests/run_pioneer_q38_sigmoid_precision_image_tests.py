#!/usr/bin/env python3
"""Pinned isolated QSG1 instruction tests, optionally original gate INPUTS."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
assert len(sys.argv) in (2,3)
here=pathlib.Path(__file__).resolve().parent;image=pathlib.Path(sys.argv[1])
PINS=(('boot.bin',4,'d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'),
 ('code.bin',2856,'4e587f314dcfba4f3df72cc01cb02e416ef2402c29793a633f75b45f9f4101d2'),
 ('program.elf',12060,'0ba8f128dc597429ad6caa1c52aeea4f9bdee6569f4cd6fef0410cfe98fbdf1e'))
for name,size,digest in PINS:
 data=(image/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
common=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='pioneer-q38-sigmoid-image-',dir='/private/tmp') as tmp:
 base=pathlib.Path(tmp);binary=base/'test'
 subprocess.run([*common,str(here/'pioneer_q38_sigmoid_precision_image_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary),str(image)],env=env,check=True)
 if len(sys.argv)==3:
  original=base/'original'
  subprocess.run([*common,'-DPIONEER_Q38_SIGMOID_IMAGE_PROBE=1',str(here/'pioneer_q38_mixed_live_original_reference_test.cpp'),
   str(here/'pioneer_q38_sigmoid_precision_host_bridge.cpp'),str(here/'pioneer_q38_sigmoid_precision_probe_bridge.cpp'),'-o',str(original)],check=True)
  subprocess.run([str(original),sys.argv[2],str(image)],env=env,check=True)
