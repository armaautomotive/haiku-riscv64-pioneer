import ast
import hashlib
import pathlib
import subprocess

out=pathlib.Path(__file__).resolve().parent
prior=pathlib.Path('/private/tmp/pioneer-q38-manual-native-20261102.20fpwL')
main=pathlib.Path('/Users/arma/Documents/ChatGPT/HaikuOS')
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for name,public in (('private',False),('public',True)):
 old=prior/('main-public-r1' if public else 'main-private-r1')
 script=main/'tools/pioneer'/('build_qwen38_token_manual_ttsync_benchmark_native.py' if public else 'build_qwen38_token_manual_ttsync_native.py')
 constants={t.id:ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body if isinstance(n,ast.Assign)
  for t in n.targets if isinstance(t,ast.Name) and t.id in ('PRIVATE_SHA','OBJECT_SHA','MODULE_SHA')}
 for label,source in (('main',main),('clean',out/'clean-source')):
  command=(old/'compile-argv.txt').read_text().splitlines()
  assert command[0]==tool
  original=pathlib.Path(command[command.index('-o')+1]);target=out/(label+'-'+name+'-current.o')
  assert sha(original)==constants['OBJECT_SHA' if public else 'PRIVATE_SHA'] and not target.exists()
  command=[str(source)+arg[len(str(main)):] if arg.startswith(str(main)+'/') else
   '-iquote'+str(source)+arg[len('-iquote'+str(main)):] if arg.startswith('-iquote'+str(main)+'/') else arg for arg in command]
  command[command.index('-o')+1]=str(target)
  subprocess.run(command,check=True)
  assert target.read_bytes()==original.read_bytes()
  if public:
   link=(old/'link-argv.txt').read_text().splitlines();previous=pathlib.Path(link[-1]);module=out/(label+'-current-module')
   assert sha(previous)==constants['MODULE_SHA'] and not module.exists() and link.count(str(original))==1
   link=[str(target) if arg==str(original) else str(module) if arg==str(previous) else arg for arg in link]
   subprocess.run(link,check=True);assert module.read_bytes()==previous.read_bytes()
  print('PASS_CURRENT_FROZEN_CANDIDATE '+label+' '+name+' '+sha(target)+';NO native/watchdog/hardware/TPS grant',flush=True)
