#!/usr/bin/env python3
"""Distinct latest-Q4-job USER diagnostic BUILD;never install/run."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA = '5e3de2b89d42b05c2d4ee03ed9d3663d283fb40d0ed8bf216cbf3d886ee60c1c'
BINARY_SHA = '559f986ddc88bbc2ff8cbe034dcda1546d385b366177f027a245452e605c7a22'
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4
assert not inspect or sys.argv[3]=='--inspect-candidate'
assert inspect or OBJECT_SHA and BINARY_SHA
build,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_token_constant_q4_client.py'),str(build),str(out/'unchanged-constant-q4-client')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
assert hashlib.sha256(old['binary'].read_bytes()).hexdigest()=='3edc924a9e76fdcd42815fa49b6d2756d80c6a18e3a3eb8bc9d4b765345ce1de'
cxx,flags,includes,libraries,release,tool=(old[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'token-matrix-phase-client.o'
command=[cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
         '-DPIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT=1','-DPIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT=1',
         *includes,'-c',str(here/'pioneer_q38_token_matrix_phase.cpp'),'-o',str(obj)]
subprocess.run(command,check=True);(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
binary=out/'pioneer-q38-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY'
link=[cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
      '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)]
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for name in ('FrontierBenchmarkTransport','PioneerQ38TokenHiddenValidationSession','PioneerQ38PublicModelSource',
             'PioneerQ38WriteFrontierFailedReply','CopyPhaseProfile','CopyMatrixPhase','ioctl'):
    assert name in symbols,name
for name in ('llama_','ggml_','OriginalSource','ReferenceProducts','PioneerSessionIO',
             'PioneerStoreContext','Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','SyntheticSource'):
    assert name not in symbols,name
data=binary.read_bytes()
for text in (b'Q38_MATRIX_PHASE_FIXED34_LATEST_Q4_JOB_HOST_LABEL_NOT_DEVICE_PROVENANCE_OR_WORK',
             b'Q38_TOKEN_PHASE_FIXED34_HELD_READ_NO_CPU_REFERENCE_NOT_WORK_RAW_CYCLES_NOT_FREQUENCY',
             b'Q38_MATRIX_PHASE_PACKET worker',b'Q38_MATRIX_PHASE_AFTER_CLEAN_END workers119',
             b'Q38_TOKEN_PHASE_AFTER_CLEAN_END words96 valid',b'--profile-q4-jobs-34-positions\0',
             b'--preflight-source-only\0',b'/bin/sha256sum\0',b'Q38_FRONTIER_FAILED_REPLY_RAW',
             b'POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF'):
    assert text in data,text
for text in (b'--benchmark-constant-q4-34-positions\0',b'--profile-card-phases-34-positions\0',
             b'--diagnose-frontier-34-positions\0',b'--benchmark-34-positions\0',b'--profile-frontends-34-positions\0',
             b'--generate-34-positions\0',b'--generate-three-positions\0',b'--capture-first-layer\0',
             b'--work\0',b'--cpu\0',b'Q38_FRONTEND_ROWS_BEGIN'):
    assert text not in data,text
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows)
graph=out/'token-matrix-phase-callgraph.o';graph_args=command[:];graph_args[graph_args.index('-o')+1]=str(graph)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True);assert graph.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import function_symbols
from pioneer_q38_formatter_graph import audit_formatter_graph
functions=function_symbols(subprocess.check_output([str(tool)+'objdump','-t',str(graph)],text=True))
focused,report=audit_formatter_graph(graph.with_suffix('.ci').read_text(),functions)
(out/'formatter-focused.ci').write_text(focused)
(out/'formatter-callgraph.json').write_text(json.dumps(report,indent=2)+'\n')
(out/'client.disassembly').write_text(subprocess.check_output([str(tool)+'objdump','-drC',str(obj)],text=True))
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('BUILD_ONLY distinct fixed34 USER sampler/whole unchanged3edc+ancestors FIRST/'
      '119 fixed held+drained peer snapshots+separate coarse phase snapshot BEFORE normal Finish/'
      'stored raw packets ONLY after actual END+Close+source check/no runtime CPU model reference,Work or MOCK imports/'
      'maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows))+
      '/formatter graph scoped ONLY,not wholeUSER stack;NO native lifecycle,physical timing,quality,Work,TPS or deployment grant',flush=True)
