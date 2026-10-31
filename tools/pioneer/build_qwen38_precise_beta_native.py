#!/usr/bin/env python3
"""Reproduce old module,then link/inspect separate precise-beta diagnostic. NEVER install."""
import hashlib
import json
import os
import pathlib
import runpy
import subprocess
import sys

job_boot_fold='--job-boot-fold' in sys.argv
if job_boot_fold:
    assert sys.argv.count('--job-boot-fold')==1
    sys.argv.remove('--job-boot-fold')
assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
previous,out=map(pathlib.Path,sys.argv[1:])
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);here=pathlib.Path(__file__).resolve().parent;repo=here.parents[1]
sys.path.insert(0,str(here/'tests'))
from audit_pioneer_q38_stack_graph import audit,function_symbols,parse_graph,symbol
saved=sys.argv[:]
try:
    sys.argv=[str(here/'tests/run_pioneer_q38_token_precise_beta_validation_native_compile.py'),str(previous),str(out/'sdk')]
    if job_boot_fold:sys.argv.append('--job-boot-fold')
    compiled=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
obj,args,tool=compiled['obj'],compiled['args'][:],compiled['tool']
old=compiled['old_object'];cxx=args[0]
# Rebuild the actual common profile from this checkout,using ONLY the old
# SHA-bound compilation flags. No guessed external trace/link command.
profile=out/'PioneerPciProfile.o';profile_args=args[:]
profile_args[profile_args.index('-c')+1]=str(repo/'src/add-ons/kernel/drivers/misc/PioneerPciProfile.cpp')
profile_args[profile_args.index('-o')+1]=str(profile)
subprocess.run(profile_args,check=True)
sdk=pathlib.Path('/Volumes/HaikuBuildLocal/generated.riscv64')
release=sdk/'objects/haiku/riscv64/release'
def compiler_file(name):
    path=pathlib.Path(subprocess.check_output([cxx,'-print-file-name='+name],text=True).strip())
    assert path.is_absolute() and path.is_file();return str(path)
begin,end,libgcc=map(compiler_file,('crtbeginS.o','crtendS.o','libgcc.a'))
link=[cxx,'-shared','-nostdlib','-Wl,--no-undefined,-soname=tenstorrent_pci_probe',
      begin,str(release/'system/glue/haiku_version_glue.o'),str(old),str(profile),
      str(release/'system/kernel/kernel.so'),libgcc,end,'-o',str(out/'old-posted-cancel-CONTROL')]
assert all(pathlib.Path(p).is_file() for p in link[4:-2])
subprocess.run(link,check=True)
control=pathlib.Path(link[-1]);old_sha=hashlib.sha256(control.read_bytes()).hexdigest()
assert old_sha=='18362bcb97180f8428b7e3b54aee7c81a42b8ed9f998aea11ccbefee5aff29af',old_sha
# The complete old linked image must reproduce before this link recipe is
# used for the candidate. Same seven inputs,only actual token object differs.
module=out/('tenstorrent_pci_probe-token-DEBUG75-JOB-BOOT-FOLD' if job_boot_fold else 'tenstorrent_pci_probe-token-DEBUG75-PRECISE-BETA')
link[6]=str(obj);link[-1]=str(module);subprocess.run(link,check=True)
# Independent main/clean link identity,NOT permission to install or run.
assert hashlib.sha256(module.read_bytes()).hexdigest()==('44209b7873cd2e2f32f133b4c4c45a5870fe1669e98cd4165c4ebc51abdc5eaf' if job_boot_fold else '2723ac08ac75996f19f6e516c51eda615336f8dcfc3a8d6849f6d755da22c525')
(out/'link-argv.txt').write_text('\n'.join(link)+'\n')
table=subprocess.check_output([tool+'nm','-C','--defined-only',str(module)],text=True)
undefined=subprocess.check_output([tool+'nm','-u',str(module)],text=True)
names={row[-1] for row in map(str.split,table.splitlines()) if row}
assert {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}<=names
for required in ('Q38TokenNativeControl','Q38TokenHiddenValidationControl','PostedCancelSourceGuard',
                 'PioneerQ38PostedCancel::Request','BeginQ38TokenNativeBackendScoped','FinishQ38TokenNativeBackendScoped',
                 'PioneerQ38TokenJobBootFoldPrograms' if job_boot_fold else 'PioneerQ38TokenPreciseBetaPrograms'):
    assert required in table,required
for forbidden in ('OriginalSource','ReferenceProducts','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PostedCancelEndpoint','SyntheticIO','pioneer_token_context_',
                  'PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined,forbidden
if job_boot_fold:
    assert 'PioneerQ38TokenPreciseBetaPrograms' not in table
graph_obj=out/('job-boot-fold-callgraph.o' if job_boot_fold else 'precise-beta-callgraph.o');graph_args=args[:]
graph_args[graph_args.index('-o')+1]=str(graph_obj)
subprocess.run([*graph_args,'-fcallgraph-info=su'],check=True)
assert graph_obj.read_bytes()==obj.read_bytes()
graph=graph_obj.with_suffix('.ci').read_bytes();nodes,edges=parse_graph(graph.decode())
roots=[key for key in nodes if symbol(key)=='_ZL12ProbeControlPvjS_m'];assert len(roots)==1
functions=function_symbols(subprocess.check_output([tool+'objdump','-t',str(graph_obj)],text=True))
results=audit(nodes,edges,functions,roots)
assert len(results)==1 and not results[0]['cycles'] and not results[0]['dynamic']
assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
report={'kind':'PARTIAL_ACTUAL_DEBUG75_JOB_BOOT_FOLD_PUBLIC_STACK_AUDIT' if job_boot_fold else 'PARTIAL_ACTUAL_DEBUG75_PRECISE_BETA_PUBLIC_STACK_AUDIT','roots':results,
        'module_sha256':hashlib.sha256(module.read_bytes()).hexdigest(),
        'object_sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),
        'old_module_reproduced_sha256':old_sha,'graph_sha256':hashlib.sha256(graph).hexdigest(),
        'whole_kernel_stack_qualified':False,
        'limits':'Known module chains only;unknown entry/kernel/IRQ/indirect costs excluded,NOT zero. '
                 'Same bounded tag75 ABI,distinct SHA-qualified diagnostic;not Work or deployment authority.'}
(out/'partial-public-entry-stack.json').write_text(json.dumps(report,indent=2)+'\n')
print(report['module_sha256']+' '+module.name)
print('PASS old18362LinkedImageReproduced1 '+('actualJobBootFoldDiagnosticLinked1' if job_boot_fold else 'actualPreciseBetaDiagnosticLinked1')+' all13ArraysAnd2MiBRopeExact1 conventionalABI1 noCPUOracleWorkOrTestImports1')
print('OBSERVED knownModuleChain'+str(results[0]['known_module_chain_bytes'])+
      '/unresolved'+str(len(results[0]['unresolved']))+'/reached'+str(results[0]['reached_nodes'])+
      ';PARTIAL ONLY,wholeKernelStackQualified=FALSE;graph object BYTE-IDENTICAL')
print('BUILD_INSPECT_ONLY_NO_INSTALL_PHYSICAL_DELIVERY_FULL_NATIVE_LIFECYCLE_MODEL_NUMERICS_WORK_OR_TPS_GRANT')
