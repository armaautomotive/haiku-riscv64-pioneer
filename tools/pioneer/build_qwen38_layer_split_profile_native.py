#!/usr/bin/env python3
"""Build isolated Haiku timing and loader-comparison candidates; no device I/O."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

from build_qwen38_head_csr_native_pair import ARRAY, DRIVER, DRIVER_DIR, RATE, TOOLS, WORKER, verify_linked_arrays

SEED='25d1533ee6a43003952828d43a88f42d6dcc3c42'
TAG='0x2026111700010213'
LIVE_TAG='0x2026111700010214'
OLD_TAG='0x2026110600010116'
MARKER='PIONEER_Q38_FAST_DECODE_SPLIT_PACKAGE'
LIVE_MARKER='PIONEER_Q38_SPAN_LIVE_IMAGE_CHECK'
STAGE='/boot/home/pioneer-q38-layer-split-candidate-213'
LIVE_STAGE='/boot/home/pioneer-q38-layer-split-live-214'
PROGRAMS='PioneerQ38FullOffloadFastDecodePrograms'
V3_PROGRAMS_SHA='3fd94232de0b045e5a74c41d59e0f80b8dba0369e54974a77e62800bf5106f8a'
V4_PROGRAMS_SHA='66938ea74e8d08a7cb66b0ecce10b09ec7ea2c80d59a0e7266a41249826fe9b1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def edit(path, changes):
    text=path.read_text()
    for old,new,count in changes:
        assert text.count(old)==count,(path,old)
        text=text.replace(old,new)
    path.write_text(text)


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    live=len(sys.argv)==6 and sys.argv[5]=='--live-image-check'
    if len(sys.argv)!=5 and not live:
        raise SystemExit('use SDK FROZEN_LLAMA_LIBRARIES V4_DIAGNOSTIC NEW_OUTPUT [--live-image-check]')
    sdk,libraries,image,out=map(Path,sys.argv[1:5])
    tag,stage=(LIVE_TAG,LIVE_STAGE) if live else (TAG,STAGE)
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
                    '--layer-split'],cwd=out,check=True)
    assert sha(generated)==V4_PROGRAMS_SHA
    replacement=generated.read_text()
    old={label:digest for digest,label in ARRAY.findall(reference)}
    expected={label:digest for digest,label in ARRAY.findall(replacement)}
    assert len(old)==len(expected)==15 and set(old)==set(expected)
    assert {n for n in expected if expected[n]!=old[n]}=={'RootText','RootAux','RootFastDecode'}
    shutil.copyfile(generated,prog)
    shutil.copyfile(here/'PioneerQ38FastDecodeProfileData.h',pioneer/'PioneerQ38FastDecodeProfileData.h')
    edit(pioneer/'PioneerQ38FastDecodeProfileData.h',[
        ('#ifdef PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE','#if 1 // isolated tag213 v4 DATA',3)])
    edit(pioneer/'PioneerQ38SelectedPhaseProfileData.h',[('0x33485051','0x34485051',1)])
    edit(pioneer/'PioneerQ38TokenSelectedPrograms.h',[('::PhaseProfileVersion==3',
        '::PhaseProfileVersion==4 && PioneerQ38TokenPrograms::LayerSplitProfile==1',1)])
    edit(drivers/'PioneerQ38FullOffloadProtocol.h',[(OLD_TAG,tag,1)])
    if live:
        shutil.copyfile(here.parents[1]/DRIVER_DIR/'PioneerQ38SpanController.h',drivers/'PioneerQ38SpanController.h')
    kernel_check=(f'#if !defined({LIVE_MARKER}) || {LIVE_MARKER} != 1\n'
                  '#error Layer split requires the selected live-image audit\n#endif\n') if live else (
                  f'#ifdef {LIVE_MARKER}\n#error Layer split requires the original full span audit\n#endif\n')
    edit(drivers/'PioneerQ38FullOffloadFastDecodeRole.h',[
        (OLD_TAG,tag,1),('actual native tag116','actual native '+label,1),
        ('#ifdef _KERNEL_MODE',f'#if !defined({MARKER}) || {MARKER} != 1 \\\n'
          ' || defined(PIONEER_Q38_LAYER_SPLIT_PROFILE_IMAGE)\n'
          f'#error Layer split requires exact separate {label} package binding\n#endif\n'
          '#ifdef _KERNEL_MODE\n'+kernel_check.rstrip(),1)])
    edit(pioneer/'pioneer_assistant_qwen27_full_offload.cpp',[
        ('Full-card fast decode profile ', 'Full-card layer split profile ',2)])
    native_script=pioneer/'build_qwen38_fast_decode_profile_native.py'
    user_script=pioneer/'build_assistant_qwen27_fast_decode_full_offload.py'
    native_args=f"'-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1', '-D{MARKER}=1'"
    if live:native_args+=f", '-D{LIVE_MARKER}=1'"
    edit(native_script,[(OLD_TAG,tag,1),(V3_PROGRAMS_SHA,V4_PROGRAMS_SHA,1),
        ('tenstorrent_pci_probe-fast-decode116',DRIVER,1),('tag116',label,2),
        ('v3 held read','v4 held read',1),
        ("'-DPIONEER_Q38_FULL_OFFLOAD_FAST_DECODE_PROFILE=1']",native_args+']',1)])
    edit(user_script,[('tag116',label,2),('v3 generation','v4 generation',1),
        ('Full-card fast decode profile ','Full-card layer split profile ',2),
        ('/boot/home/pioneer-q38-fast-decode-116-a',stage,1),
        ('HaikuAssistantQwen27FastDecodeFullOffload',WORKER,2),('pioneer-q38-fast-decode-rate-smoke',RATE,1),
        ("compile = old['compile'][:]",f"compile = old['compile'][:]\ncompile.insert(1, '-D{MARKER}=1')",1)])

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
        if work==native:
            cases.append(['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG='+OLD_TAG+'ULL'])
            cases.append(['-U'+LIVE_MARKER] if live else ['-D'+LIVE_MARKER+'=1'])
            if live:cases.append(['-DPIONEER_Q38_SPAN_REUSE_CODE=1'])
        else:cases.append(['-DPIONEER_Q38_WORK_INFERENCE=1'])
        for flags in cases:
            refused=subprocess.run([*args,*flags,'-fsyntax-only',source],cwd=work,capture_output=True,text=True)
            assert refused.returncode and any(msg in refused.stderr for msg in
                ('Layer split requires','Fast decode profile requires','Live executable image audit requires')),flags
            failures+=1
    manifest={'tag':tag,'stage':stage,'source':SEED,'program_header':sha(generated),'profile_version':4,
              'artifacts':{p.name:sha(p) for p in (native/DRIVER,user/WORKER,user/RATE)},'linked_arrays':arrays}
    if live:manifest['live_image_check']=True
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    print(f'PASS BUILD ONLY {label}: 15 final linked arrays, identical ten vector92 peers, v4 held timing, '
          f'{failures} extra role refusals; no installed, model or physical rate result',flush=True)


if __name__=='__main__':
    main()
