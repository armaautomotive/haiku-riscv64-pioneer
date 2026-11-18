#!/usr/bin/env python3
"""Build isolated Haiku timing and loader-comparison candidates; no device I/O."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

from build_qwen38_head_csr_native_pair import ARRAY, DRIVER, DRIVER_DIR, RATE, TOOLS, WORKER, verify_linked_arrays
from build_qwen38_twin_native_q8_wire import insert as insert_wire

SEED='25d1533ee6a43003952828d43a88f42d6dcc3c42'
TAG='0x2026111700010213'
LIVE_TAG='0x2026111700010214'
RECIPE_TAG='0x2026111700010216'
WIRE_TAG='0x2026111700010217'
RAW_HEAD_TAG='0x2026111700010218'
WIRE_HEAD_TAG='0x2026111700010219'
OLD_TAG='0x2026110600010116'
MARKER='PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE'
LIVE_MARKER='PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK'
RECIPE_MARKER='PIONEER_Q38_RECIPE_BATCH_PACKAGE'
RECIPE_ROOT_MARKER='PIONEER_Q38_TOKEN_RECIPE_BATCH_IMAGE'
WIRE_MARKER='PIONEER_Q38_NATIVE_Q8_FULL_PACKAGE'
WIRE_IMAGE_MARKER='PIONEER_Q38_TWIN_NATIVE_Q8_WIRE'
HEAD_MARKER='PIONEER_Q38_HEAD_BATCH_V4_PACKAGE'
STAGE='/boot/home/pioneer-q38-layer-split-candidate-213'
LIVE_STAGE='/boot/home/pioneer-q38-layer-split-live-214'
RECIPE_STAGE='/boot/home/pioneer-q38-layer-split-recipe-216'
WIRE_STAGE='/boot/home/pioneer-q38-native-q8-wire-217'
RAW_HEAD_STAGE='/boot/home/pioneer-q38-profile-head-raw-218'
WIRE_HEAD_STAGE='/boot/home/pioneer-q38-profile-head-wire-219'
PROGRAMS='PioneerQ38FullOffloadFastDecodePrograms'
V3_PROGRAMS_SHA='3fd94232de0b045e5a74c41d59e0f80b8dba0369e54974a77e62800bf5106f8a'
V4_PROGRAMS_SHA='66938ea74e8d08a7cb66b0ecce10b09ec7ea2c80d59a0e7266a41249826fe9b1'
RECIPE_PROGRAMS_SHA='e7e5866a747ad7e40651889c0a254cb474d09b3073f0ee65b828b080095354dc'
WIRE_PROGRAMS_SHA='50ff3bcf5ae57f265af55be9d72eea750c819c13d23949cb6a4a9f0149c590f7'
RAW_HEAD_SHA='a59a15d61e4c081c642194a57600fc42841279ec708061c40a043b3a399796c2'
WIRE_HEAD_SHA='a82e965673d26871565ac088d7a15f283c8a2b49269a9ec146c579cd3c26dc9b'
MODES={
    '--layer-split':(TAG,STAGE,V4_PROGRAMS_SHA),
    '--live-image-check':(LIVE_TAG,LIVE_STAGE,V4_PROGRAMS_SHA),
    '--recipe-batch':(RECIPE_TAG,RECIPE_STAGE,RECIPE_PROGRAMS_SHA),
    '--native-q8-wire':(WIRE_TAG,WIRE_STAGE,WIRE_PROGRAMS_SHA),
    '--head-batch-v4':(RAW_HEAD_TAG,RAW_HEAD_STAGE,RAW_HEAD_SHA),
    '--native-q8-head-batch-v4':(WIRE_HEAD_TAG,WIRE_HEAD_STAGE,WIRE_HEAD_SHA),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def edit(path, changes):
    text=path.read_text()
    for old,new,count in changes:
        assert text.count(old)==count,(path,old)
        text=text.replace(old,new)
    path.write_text(text)


def verify_head_journal(pioneer,drivers,image,out,tag,wire):
    # Execute the same native journal template against simulated held L1
    # and test the exact first/last copied, audited and restored addresses.
    gate=(drivers/'PioneerQ38FullOffloadRole.h').read_text()
    required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',gate))
    assert len(required)==23
    flags=dict.fromkeys(required,'1')
    flags.update(PIONEER_DRIVER_BUILD_TAG=tag+'ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
                 PIONEER_Q38_FULL_OFFLOAD_METRICS='1',PIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE='1')
    for name in (MARKER,HEAD_MARKER,LIVE_MARKER):flags[name]='1'
    if wire:
        flags[WIRE_MARKER]='1';flags[WIRE_IMAGE_MARKER]='1'
    (out/'fast_decode_native_role_defines.inc').write_text(''.join(f'#define {k} {v}\n' for k,v in sorted(flags.items())))
    for kind,args in (('data',[image/'batch']),('memory',[])):
        name=f'pioneer_q38_fast_decode_native_{kind}_test.cpp'
        test=pioneer/'tests'/name;shutil.copyfile(Path(__file__).with_name('tests')/name,test)
        binary=out/('head-journal-'+kind)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-I'+str(out),test,'-o',binary],check=True)
        subprocess.run([binary,*args],check=True,timeout=240,
                       env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'quarantine_size_mb=32'})


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    mode=sys.argv[5] if len(sys.argv)==6 else '--layer-split'
    if len(sys.argv) not in (5,6) or mode not in MODES:
        raise SystemExit('use SDK FROZEN_LLAMA_LIBRARIES ROOT_DIAGNOSTIC_OR_WIRE_PAIR NEW_OUTPUT ['
                         +'|'.join(MODES)+']')
    recipe=mode=='--recipe-batch'
    wire=mode in ('--native-q8-wire','--native-q8-head-batch-v4')
    head=mode in ('--head-batch-v4','--native-q8-head-batch-v4')
    fast_upload=(mode=='--live-image-check') or wire or head
    sdk,libraries,image,out=map(Path,sys.argv[1:5])
    tag,stage,programs_sha=MODES[mode]
    label='tag'+tag[-3:]
    assert all(p.is_absolute() for p in (sdk,libraries,image,out))
    assert not out.exists() and not out.is_symlink()
    here=Path(__file__).resolve().parent
    repo=here.parents[1]
    out.mkdir(mode=0o700)
    tree=out/'source';tree.mkdir(mode=0o700)
    paths=(TOOLS,DRIVER_DIR,'src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h',
           'src/apps/assistant','headers','build/config_headers')
    with subprocess.Popen(['git','-C',repo,'archive',SEED,*paths],stdout=subprocess.PIPE) as archive:
        with archive.stdout:
            subprocess.run(['tar','-x','-C',tree],stdin=archive.stdout,check=True)
        assert archive.wait()==0
    (tree/'src/libs').mkdir()
    (tree/'src/libs/llama.cpp').symlink_to(repo/'src/libs/llama.cpp',target_is_directory=True)
    pioneer,drivers=tree/TOOLS,tree/DRIVER_DIR
    prog=pioneer/(PROGRAMS+'.h')
    assert sha(prog)==V3_PROGRAMS_SHA
    reference=prog.read_text()
    generated=out/'split-programs.h'
    subprocess.run([sys.executable,here/'generate_qwen38_fast_decode_profile_programs.py',image,generated,
                    '--layer-split' if mode=='--live-image-check' else mode],cwd=out,check=True)
    assert sha(generated)==programs_sha
    replacement=generated.read_text()
    old={label:digest for digest,label in ARRAY.findall(reference)}
    expected={label:digest for digest,label in ARRAY.findall(replacement)}
    assert len(old)==len(expected)==15 and set(old)==set(expected)
    changed={'RootText','RootAux','RootFastDecode'}
    if wire:changed|={n for n in old if n.startswith('Peer') and n!='PeerBoot'}
    assert {n for n in expected if expected[n]!=old[n]}==changed
    shutil.copyfile(generated,prog)
    shutil.copyfile(here/'PioneerQ38FastDecodeProfileData.h',pioneer/'PioneerQ38FastDecodeProfileData.h')
    edit(pioneer/'PioneerQ38FastDecodeProfileData.h',[
        ('#ifdef PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE','#if 1 // isolated tag213 v4 DATA',3)])
    edit(pioneer/'PioneerQ38SelectedPhaseProfileData.h',[('0x33485051','0x34485051',1)])
    selector='::PhaseProfileVersion==4 && PioneerQ38TokenPrograms::LayerSplitProfile==1'
    if recipe:selector+=' && PioneerQ38TokenPrograms::RecipeBatchProfile==1'
    if wire:selector+=' && PioneerQ38TokenPrograms::NativeQ8Wire==1'
    if head:selector+=(' && PioneerQ38TokenPrograms::HeadBatchV4==1 '
                       '&& sizeof(PioneerQ38TokenPrograms::RootFastDecode)==4*PioneerQ38TokenL1::RootFastDecodeWords')
    edit(pioneer/'PioneerQ38TokenSelectedPrograms.h',[('::PhaseProfileVersion==3',selector,1)])
    edit(drivers/'PioneerQ38FullOffloadProtocol.h',[(OLD_TAG,tag,1)])
    if fast_upload:
        shutil.copyfile(here.parents[1]/DRIVER_DIR/'PioneerQ38SpanController.h',drivers/'PioneerQ38SpanController.h')
    if wire or head:
        for name in ('PioneerQ38NativeQ8FullRole.h','PioneerQ38HeadBatchV4Role.h','PioneerQ38FullOffloadFastDecodeRole.h'):
            shutil.copyfile(repo/DRIVER_DIR/name,drivers/name)
        session='PioneerQ38FullOffloadSession.h'
        anchor='#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"\n'
        following='#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"\n'
        tracked=(here/session).read_text()
        assert tracked.count(anchor)==tracked.count(following)==1
        block=tracked.split(anchor,1)[1].split(following,1)[0]
        assert block.startswith('#ifdef '+WIRE_MARKER+'\nstatic_assert(')
        edit(pioneer/session,[(anchor,anchor+block,1)])
    if head:
        shutil.copyfile(here/'PioneerQ38TokenPushedFootprint.h',pioneer/'PioneerQ38TokenPushedFootprint.h')
        journal=' && PioneerQ38TokenL1::PreparedQ4==0 && PioneerQ38TokenL1::SavedBytes==53915328,\n'
        edit(drivers/'PioneerQ38FullOffloadRole.h',[
            (journal,journal.replace('53915328','53916484'),1),
            ('"fast decode ROOT code must be journaled exactly through6c5d8"',
             '"v4 head batch ROOT code must be journaled exactly through6ca5c"',1)])
    if wire:
        for name in ('PioneerQ38NativeQ8FullSource.h','PioneerQ38PublicModelSource.h',
                     'PioneerQ38PushedWorkerLayout.h','PioneerQ38TwinNativeQ8Wire.h'):
            shutil.copyfile(here/name,pioneer/name)
        shutil.copyfile(repo/DRIVER_DIR/'PioneerQ38SpanProtocol.h',drivers/'PioneerQ38SpanProtocol.h')
        insert_wire(pioneer,'PioneerQ38SelectedGlobalWorker.h',
                    'namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PushedWorkerLayout;\n',
                    ',PioneerQ38SelectedWorkerHybrid=0')
    kernel_check=(f'#if !defined({LIVE_MARKER}) || {LIVE_MARKER} != 1\n'
                  '#error Layer split requires the selected live-image audit\n#endif\n') if fast_upload else (
                  f'#ifdef {LIVE_MARKER}\n#error Layer split requires the original full span audit\n#endif\n')
    recipe_check=(f'#if !defined({RECIPE_MARKER}) || {RECIPE_MARKER} != 1\n'
                  '#error Layer split requires the exact recipe-batch package selector\n#endif\n') if recipe else (
                  f'#ifdef {RECIPE_MARKER}\n#error Layer split requires the original serial recipe sender\n#endif\n')
    wire_check=(f'#if !defined({WIRE_MARKER}) || {WIRE_MARKER} != 1\n'
                '#error Layer split requires the exact native Q8 package selector\n#endif\n') if wire else (
                f'#ifdef {WIRE_MARKER}\n#error Layer split requires its original weight format\n#endif\n')
    head_check=(f'#if !defined({HEAD_MARKER}) || {HEAD_MARKER} != 1\n'
                '#error Layer split requires the exact V4 head batch journal selector\n#endif\n') if head else (
                f'#ifdef {HEAD_MARKER}\n#error Layer split requires its original journal\n#endif\n')
    edit(drivers/'PioneerQ38FullOffloadFastDecodeRole.h',[
        (OLD_TAG,tag,1),('actual native tag116','actual native '+label,1),
        ('#ifdef _KERNEL_MODE',f'#if !defined({MARKER}) || {MARKER} != 1 \\\n'
          f' || defined(PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE) || defined({RECIPE_ROOT_MARKER})\n'
          f'#error Layer split requires exact separate {label} package binding\n#endif\n'
          +recipe_check+wire_check+head_check+
          '#ifdef _KERNEL_MODE\n'+kernel_check.rstrip(),1)])
    edit(pioneer/'pioneer_assistant_qwen27_full_offload.cpp',[
        ('Full-card fast decode profile ', 'Full-card layer split profile ',2)])
    native_script=pioneer/'build_qwen38_fast_decode_profile_native.py'
    user_script=pioneer/'build_assistant_qwen27_fast_decode_full_offload.py'
    user_args=f"'-D{MARKER}=1'"
    if recipe:user_args+=f", '-D{RECIPE_MARKER}=1'"
    if wire:user_args+=f", '-D{WIRE_MARKER}=1'"
    if head:user_args+=f", '-D{HEAD_MARKER}=1'"
    native_args=f"'-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1', {user_args}"
    if fast_upload:native_args+=f", '-D{LIVE_MARKER}=1'"
    if wire:native_args+=f", '-D{WIRE_IMAGE_MARKER}=1'"
    edit(native_script,[(OLD_TAG,tag,1),(V3_PROGRAMS_SHA,programs_sha,1),
        ('tenstorrent_pci_probe-fast-decode116',DRIVER,1),('tag116',label,2),
        ('v3 held read','v4 held read',1),
        ("'-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1']",native_args+']',1)])
    if head:edit(native_script,[('assert size == 1496','assert size == 2652',1),('53915328B journal','53916484B journal',1)])
    edit(user_script,[('tag116',label,2),('v3 generation','v4 generation',1),
        ('Full-card fast decode profile ','Full-card layer split profile ',2),
        ('/boot/home/pioneer-q38-fast-decode-116-a',stage,1),
        ('HaikuAssistantQwen27FastDecodeFullOffload',WORKER,2),('pioneer-q38-fast-decode-rate-smoke',RATE,1),
        ("compile = old['compile'][:]",f"compile = old['compile'][:]\ncompile[1:1] = [{user_args}]",1)])

    def build(name,script,args):
        work=out/(name+'-work');work.mkdir(mode=0o700)
        subprocess.run([sys.executable,script,*args],cwd=work,check=True)
    native,user=out/'native',out/'user'
    with ThreadPoolExecutor(max_workers=2) as pool:
        calls=(pool.submit(build,'native',native_script,[sdk,native]),
               pool.submit(build,'user',user_script,[sdk,libraries,user]))
        for call in calls:call.result()

    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    arrays=verify_linked_arrays(tool,native,expected,namespace=PROGRAMS)
    failures=0
    for work in (native,user):
        command=json.loads((work/'build-argv.json').read_text())[0]
        args=command[:command.index('-c')]
        source=command[command.index('-c')+1]
        cases=[['-U'+MARKER],['-U'+MARKER,'-D'+MARKER+'=0'],['-DPIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE=1']]
        cases.append(['-D'+RECIPE_ROOT_MARKER+'=0'])
        if recipe:cases.extend((['-U'+RECIPE_MARKER],['-U'+RECIPE_MARKER,'-D'+RECIPE_MARKER+'=0']))
        else:cases.append(['-D'+RECIPE_MARKER+'=1'])
        if work==native:
            cases.append(['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG='+OLD_TAG+'ULL'])
            cases.append(['-U'+LIVE_MARKER] if fast_upload else ['-D'+LIVE_MARKER+'=1'])
            if fast_upload:cases.append(['-DPIONEER_Q38_SPAN_REUSE_CODE=1'])
            if wire:cases.append(['-U'+WIRE_IMAGE_MARKER])
        else:cases.append(['-DPIONEER_Q38_WORK_INFERENCE=1'])
        if wire:
            cases.extend((['-U'+WIRE_MARKER],['-U'+WIRE_MARKER,'-D'+WIRE_MARKER+'=0'],['-DPIONEER_Q38_NATIVE_Q8_LAYOUT=1']))
        if head:
            cases.extend((['-U'+HEAD_MARKER],['-U'+HEAD_MARKER,'-D'+HEAD_MARKER+'=0'],
                          ['-DPIONEER_Q38_HEAD_CSR_BATCH_ROOT_IMAGE=1']))
            if work==native:cases.append(['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG='
                                          +(RAW_HEAD_TAG if wire else WIRE_HEAD_TAG)+'ULL'])
        for flags in cases:
            refused=subprocess.run([*args,*flags,'-fsyntax-only',source],cwd=work,capture_output=True,text=True)
            assert refused.returncode and any(msg in refused.stderr for msg in
                ('Layer split requires','Fast decode profile requires','Live executable image audit requires',
                 'Native Q8 full-card package requires','Native Q8 card programs cannot','V4 head batch requires')),flags
            failures+=1
    manifest={'tag':tag,'stage':stage,'source':SEED,'program_header':sha(generated),'profile_version':4,
              'artifacts':{p.name:sha(p) for p in (native/DRIVER,user/WORKER,user/RATE)},'linked_arrays':arrays}
    if fast_upload:manifest['live_image_check']=True
    if recipe:manifest['recipe_batch']=True
    if head:manifest.update(head_batch_v4=True,root_aux_program_bytes=2652,journal_bytes=53916484)
    if head:verify_head_journal(pioneer,drivers,image,out,tag,wire)
    if wire:
        manifest['native_q8_wire']=True
        name='pioneer_q38_native_q8_startup_test.cpp'
        test=pioneer/'tests'/name;shutil.copyfile(here/'tests'/name,test)
        binary=out/'startup-test'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined',test,'-o',binary],check=True)
        subprocess.run([binary],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    peers='matched native Q8 wire peers' if wire else 'identical ten vector92 peers'
    print(f'PASS BUILD ONLY {label}: 15 final linked arrays, {peers}, v4 held timing, '
          f'{failures} extra role refusals; no installed, model or physical rate result',flush=True)


if __name__=='__main__':
    main()
