#!/usr/bin/env python3
"""No input file mutation: prove code emission uses its authenticated bytes."""
import ast
import contextlib
import io
import pathlib
import runpy
import sys
import tempfile
assert len(sys.argv)==3
here=pathlib.Path(__file__).resolve().parent;root,worker=map(pathlib.Path,sys.argv[1:])
def records(path):
 tree=ast.parse(path.read_text())
 return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
expected={(directory/name) for directory,pins in ((root,records(here/'run_pioneer_q38_recurrent_prefix_root_tests.py')),
 (worker,records(here/'run_pioneer_q38_mixed_worker_tests.py'))) for name,_,_ in pins}
generator=here.parent/'generate_qwen38_recurrent_prefix_programs.py'
original_read=pathlib.Path.read_bytes;saved_argv=sys.argv[:]
with tempfile.TemporaryDirectory(prefix='pioneer-prefix-program-snapshot-',dir='/private/tmp') as tmp:
 output=pathlib.Path(tmp)/'snapshot.h';counts={}
 def changed_after_auth(path):
  data=original_read(path)
  if path in expected:
   counts[path]=counts.get(path,0)+1
   if counts[path]>1:return bytes([data[0]^1])+data[1:]
  return data
 try:
  pathlib.Path.read_bytes=changed_after_auth;sys.argv=[str(generator),str(root),str(worker),str(output)]
  with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(str(generator),run_name='__main__')
 finally:pathlib.Path.read_bytes=original_read;sys.argv=saved_argv
 assert set(counts)==expected and all(value==1 for value in counts.values())
 assert output.read_bytes()==(here.parent/'PioneerQ38RecurrentPrefixPrograms.h').read_bytes()
 for index,target in enumerate(sorted(expected)):
  refused=pathlib.Path(tmp)/('refused'+str(index)+'.h')
  def invalid_first_pin(path):
   data=original_read(path)
   return bytes([data[0]^1])+data[1:] if path==target else data
  try:
   pathlib.Path.read_bytes=invalid_first_pin;sys.argv=[str(generator),str(root),str(worker),str(refused)]
   with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(str(generator),run_name='__main__')
  except AssertionError:pass
  else:raise AssertionError('changed artifact unexpectedly emitted')
  finally:pathlib.Path.read_bytes=original_read;sys.argv=saved_argv
  assert not refused.exists()
 print('PASS pinned prefix program snapshot/SAME authenticated immutable bytes emitted/'+str(len(expected))+' inputs read ONCE/'+str(len(expected))+' changed-pin refusals BEFORE output creation/input files UNCHANGED;NO device IO')
