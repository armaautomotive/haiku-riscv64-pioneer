import hashlib
import pathlib
import subprocess

out=pathlib.Path(__file__).resolve().parent
prior=pathlib.Path('/private/tmp/pioneer-q38-manual-native-20261102.20fpwL/main-client-final')
main=pathlib.Path('/Users/arma/Documents/ChatGPT/HaikuOS')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for label,source in (('main',main),('clean',out/'clean-source')):
 command=(prior/'compile-argv.txt').read_text().splitlines()
 old=pathlib.Path(command[command.index('-o')+1]);obj=out/(label+'-current-client.o')
 assert sha(old)=='e73dbf7df8328c87abaea3689d0e2eea5f588f25e8dd413c07f235c6041a39e7' and not obj.exists()
 command=[str(source)+arg[len(str(main)):] if arg.startswith(str(main)+'/') else arg for arg in command]
 command[command.index('-o')+1]=str(obj);subprocess.run(command,check=True)
 assert obj.read_bytes()==old.read_bytes()
 link=(prior/'link-argv.txt').read_text().splitlines();oldbin=pathlib.Path(link[-1]);binary=out/(label+'-current-client')
 assert sha(oldbin)=='aaffcaa125a02d1629dc49055d45d26af28f3187049dda631a8b5a272a9c165a' and not binary.exists()
 assert link.count(str(old))==1
 link=[str(obj) if arg==str(old) else str(binary) if arg==str(oldbin) else arg for arg in link]
 subprocess.run(link,check=True);assert binary.read_bytes()==oldbin.read_bytes()
 print('PASS_CURRENT_FROZEN_USER '+label+' '+sha(obj)+' '+sha(binary)+';NO native/watchdog/hardware/TPS grant',flush=True)
