#!/usr/bin/env python3
"""Exercise ACTUAL extracted fork/wait/log/control code with fake CPU graph."""
import os,pathlib,signal,subprocess,sys,tempfile,time
binary=sys.argv[1]
cases=0
def run(scenario,commands,expected,children):
 global cases
 with tempfile.TemporaryDirectory(prefix='pioneer-q38-host-process-') as name:
  root=pathlib.Path(name);source=root/'fixture'
  source.write_bytes(bytes((i*29+17)%256 for i in range(20000)))
  env={**os.environ,'TEST_SCENARIO':scenario,'TEST_SOURCE':str(source),'TEST_DIRECTORY':str(root/'children.XXXXXX')}
  p=subprocess.run([binary,'--public-ffn-command-host'],input=commands,env=env,text=True,capture_output=True,timeout=10)
  assert p.returncode==expected,(scenario,p.returncode,p.stdout,p.stderr)
  assert p.stdout.count('BENCHMARK_CHILD_STARTED ')==children,(scenario,p.stdout)
  assert sum(len(list(d.glob('round*.log'))) for d in root.glob('children.*'))==children
  if expected:assert 'AWAIT_EXPLICIT_MEASURE' not in p.stdout
  else:assert p.stdout.count('child_known_clean1')==children
  cases+=1
run('', 'measure\n'*6+'quit\n',0,6)
run('', 'measure\n'*6,0,6) # EOF while idle,not a retry
run('', 'quit\n',0,0)
for command in ('measure','MEASURE\n','measure \n','\n','x'*80+'\n','quit\r\n'):
 run('',command,1,0)
for scenario in ('graph-failed','killed','not-opened','session-failed','finish-failed','source-changed'):
 run(scenario,'measure\nmeasure\nquit\n',1,1)
run('authentication','measure\n',1,0)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-host-interrupt-') as name:
 root=pathlib.Path(name);source=root/'fixture';source.write_bytes(bytes(20000))
 p=subprocess.Popen([binary,'--public-ffn-command-host'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1,
  env={**os.environ,'TEST_SCENARIO':'signal','TEST_SOURCE':str(source),'TEST_DIRECTORY':str(root/'children.XXXXXX')})
 p.stdin.write('measure\nmeasure\n');p.stdin.flush();lines=[]
 while True:
  line=p.stdout.readline();assert line,(lines,p.stderr.read());lines.append(line)
  if 'BENCHMARK_CHILD_STARTED ' in line:break
 # Wait for child signal handler inherited from main;do not race child entry.
 time.sleep(.05);p.send_signal(signal.SIGTERM);out,err=p.communicate(timeout=10)
 assert p.returncode==1 and 'STOP_BENCHMARK_CHILD_FAILED_OR_LOST' in out,(p.returncode,out,err)
 assert ''.join(lines).count('BENCHMARK_CHILD_STARTED ')==1 and 'AWAIT_EXPLICIT_MEASURE' not in out
 cases+=1
print(f'PASS {cases} ACTUAL extracted parent/fork/wait/barrier/log/control cases:6 fresh children twice;shared stdio offset;bad/partial control;child death/failure/source change/SIGTERM stop WITHOUT another child;CPU mocks NOT native authentication/GPU')
