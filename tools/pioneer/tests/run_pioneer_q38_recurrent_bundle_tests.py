#!/usr/bin/env python3
"""Rebuild the pinned recurrent program bundle; never open a device."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

assert __debug__ and len(sys.argv) in (4,5) and (len(sys.argv)==4 or sys.argv[4]=='--ffn-reuse') and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
root,worker,old_root=map(Path,sys.argv[1:4]);assert all(p.is_absolute() and p.is_dir() for p in (root,worker,old_root))
ffn_reuse=len(sys.argv)==5
selector='--qgwf-recurrent-ffn-reuse-inspection' if ffn_reuse else '--qgwf-recurrent-native-inspection'
digest='0c3c8a420ae424cbda4137a41cbf684e45557c9663bfa78f11d1affb6756fcc8' if ffn_reuse else '8380e513a2af99644130f293b8a3239109ec42654daa950db482e2a3b20c5b82'
generator=Path(__file__).resolve().parent.parent/'generate_qwen38_hybrid_programs.py'
with tempfile.TemporaryDirectory(prefix='q38-recurrent-bundle-',dir='/private/tmp') as tmp:
    folder=Path(tmp);outputs=[folder/'a.h',folder/'b.h']
    for out in outputs:
        subprocess.run([sys.executable,str(generator),selector,str(root),str(worker),str(out)],check=True)
    assert outputs[0].read_bytes()==outputs[1].read_bytes()
    assert hashlib.sha256(outputs[0].read_bytes()).hexdigest()==digest
    for name,options,bad_root in (
        ('old.h',[selector],old_root),
        ('ambiguous.h',[selector,'--qgwf-frontier-native-inspection'],root),
    ):
        out=folder/name
        result=subprocess.run([sys.executable,str(generator),*options,str(bad_root),str(worker),str(out)],capture_output=True)
        assert result.returncode and not out.exists(),name
print('PASS recurrent ROOT/worker bundle reproducibility and mismatched-image refusal; NO DEVICE')
