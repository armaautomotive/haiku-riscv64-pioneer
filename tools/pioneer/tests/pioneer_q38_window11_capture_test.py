#!/usr/bin/env python3
"""Capture wrapper checks with inert /usr/bin/true and false only."""
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv)==1
wrapper=Path(__file__).with_name('capture_pioneer_q38_window11_compare.py')
with tempfile.TemporaryDirectory(prefix='pioneer-q38-capture-',dir='/private/tmp') as name:
 root=Path(name);model=root/'nonexistent-model';success=root/'success';failure=root/'failure'
 def run(binary,log):
  return subprocess.run([sys.executable,str(wrapper),binary,str(model),str(log)],capture_output=True,text=True)
 good=run('/usr/bin/true',success);assert good.returncode==0 and success.read_text()==good.stdout
 assert success.read_text().endswith('CAPTURE_EXIT 0\n') and success.stat().st_mode&0o777==0o600
 original=success.read_bytes();again=run('/usr/bin/true',success)
 assert again.returncode and success.read_bytes()==original
 bad=run('/usr/bin/false',failure);assert bad.returncode and failure.read_text()==bad.stdout
 assert failure.read_text().endswith('CAPTURE_EXIT 1\n')
 invalid=subprocess.run([sys.executable,str(wrapper),'relative',str(model),str(root/'unused')],capture_output=True)
 assert invalid.returncode and not (root/'unused').exists()
print('PASS capture complete stdout, child failure, private new-file/no-overwrite and argument refusals; no model or device')
