#!/usr/bin/env python3
"""Separate returned-failure-packet USER client BUILD;NEVER install/run."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

OBJECT_SHA = '60775f89c05e0aedce37c73247999b70c5b1bbdb6805f32d6640a84228f50bbd'
BINARY_SHA = 'f8efe67a51d9685a9e7e3ce57267ed2ae4967600c2802dd21f5f6ee0c24a22c9'
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4
assert not inspect or sys.argv[3]=='--inspect-candidate'
assert inspect or OBJECT_SHA and BINARY_SHA
build,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert build.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_frontend_profile_client.py'),str(build),str(out/'unchanged-profile-client')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
# ENTIRE old718d+4b+055+allshort/timed/hidden controls FIRST,every build.
cxx,flags,includes,libraries,release,tool=(old[k] for k in ('cxx','flags','includes','libraries','release','tool'))
obj=out/'frontier-failure-report-client.o'
command=[cxx,*flags,'-DPIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT=1',
         '-DPIONEER_Q38_TOKEN_DECODE_BENCHMARK_CLIENT=1','-DPIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_CLIENT=1',
         *includes,'-c',str(here/'pioneer_q38_frontier_failure_report.cpp'),'-o',str(obj)]
subprocess.run(command,check=True)
(out/'compile-argv.txt').write_text('\n'.join(command)+'\n')
binary=out/'pioneer-q38-FRONTIER34-FAILED-REPLY-DIAGNOSTIC-ONLY'
link=[cxx,*includes,*libraries,'-B'+str(release/'system/glue')+'/',
      '-L'+str(release/'system/libroot'),str(obj),'-o',str(binary)]
subprocess.run(link,check=True);(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
symbols=subprocess.check_output([str(tool)+'nm','-C',str(binary)],text=True)
for required in ('FrontierBenchmarkTransport','PioneerQ38TokenHiddenValidationSession',
                 'PioneerQ38PublicModelSource','PioneerQ38WriteFrontierFailedReply','ioctl'):
    assert required in symbols,required
for forbidden in ('llama_','ggml_','OriginalSource','ReferenceProducts','PioneerSessionIO',
                  'PioneerStoreContext','Q38WorkControl','NativeQ38PeerInstructionModel','SyntheticIO','SyntheticSource'):
    assert forbidden not in symbols,forbidden
data=binary.read_bytes()
for required in (b'--diagnose-frontier-34-positions\0',b'--preflight-source-only\0',b'/bin/sha256sum\0',
                 b'Q38_FRONTIER_FAILURE_REPORT_ZERO_EXTRA_ACCELERATOR_CALLS_RAW_REPLY_NOT_ROOT_CAUSE_OR_CLEAN_END',
                 b'Q38_FRONTIER_FAILED_REPLY_RAW',b'Q38_FRONTIER_FAILED_PACKET word',b'Q38_FRONTIER_FAILED_FIELDS',
                 b'ALREADY_RETURNED_DATA_NOT_PROVENANCE_ROOT_CAUSE_OR_CLEAN_END',
                 b'POST_PROMPT_COUNT_WINDOW_NOT_ISOLATED_TOKEN_LATENCY_OR_QUALITY_PROOF'):
    assert required in data,required
for forbidden in (b'--benchmark-34-positions\0',b'--profile-frontends-34-positions\0',
                  b'--generate-34-positions\0',b'--generate-three-positions\0',b'--capture-first-layer\0',
                  b'--work\0',b'--cpu\0',b'Q38_FRONTEND_ROWS_BEGIN'):
    assert forbidden not in data,forbidden
rows=[row.split('\t') for row in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(row)==3 and row[2]=='static' and int(row[1])<=32768 for row in rows)
# Recompile actual code with GCC graph metadata;this must NOT change bytes.
graph=out/'frontier-failure-report-callgraph.o';graph_args=command[:]
graph_args[graph_args.index('-o')+1]=str(graph)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True)
assert graph.read_bytes()==obj.read_bytes()
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import function_symbols
from pioneer_q38_formatter_graph import audit_formatter_graph
functions=function_symbols(subprocess.check_output([str(tool)+'objdump','-t',str(graph)],text=True))
focused,report=audit_formatter_graph(graph.with_suffix('.ci').read_text(),functions)
(out/'formatter-focused.ci').write_text(focused)
(out/'formatter-callgraph.json').write_text(json.dumps(report,indent=2)+'\n')
print('FORMATTER_ACTUAL_GRAPH knownBytes'+str(report['roots'][0]['known_module_chain_bytes'])+
      '/unresolved'+','.join(report['roots'][0]['unresolved'])+';NO accelerator/control edges;NOT wholeUSER stack',flush=True)
disassembly=subprocess.check_output([str(tool)+'objdump','-drC',str(obj)],text=True)
(out/'client.disassembly').write_text(disassembly)
for path,expected in ((obj,OBJECT_SHA),(binary,BINARY_SHA)):
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:assert digest==expected,(path,digest)
    print(digest+' '+path.name,flush=True)
print('BUILD_ONLY distinct fixed34 failed-reply USER client/ENTIREold718d+4b+055+short+timed+hidden FIRST/'
      'same original ioctl+failure return+close-only lifecycle/noCPU oracle,native,Work or MOCK imports/'
      'maxIndividualUSERFrame'+str(max(int(row[1]) for row in rows)),flush=True)
print('RAW caller-owned packet ONLY after ioctl failure;NO additional accelerator call,retry,Cancel,HOLD,END,'
      'root-cause/provenance/cleanup/model-quality/speed or deployment grant',flush=True)
