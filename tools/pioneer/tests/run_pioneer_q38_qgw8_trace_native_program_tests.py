#!/usr/bin/env python3
"""Build-only trace program identity and immutable code checks; no driver."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) in (5,7) and (len(sys.argv)==5 or sys.argv[5]=='--sdk') and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
baseline, trace_a, trace_b, worker=map(Path,sys.argv[1:5])
sdk=Path(sys.argv[6]) if len(sys.argv)==7 else None
assert sdk is None or sdk.is_absolute() and sdk.is_dir()
assert all(p.is_absolute() and p.is_dir() for p in (baseline,trace_a,trace_b,worker))
here=Path(__file__).resolve().parent;root=here.parent;compiler=os.environ.get('CXX','c++')
generator=root/'generate_qwen38_hybrid_programs.py'
required='''PIONEER_Q38_QGW8_TRACE_NATIVE_131 _KERNEL_MODE PIONEER_Q38_QGW8_CANDIDATE_NATIVE_128
PIONEER_Q38_QGW8_NATIVE_INSPECTION PIONEER_Q38_HYBRID_FULL_NATIVE PIONEER_Q38_FULL_OFFLOAD_NATIVE
PIONEER_Q38_TOKEN_NATIVE_CONTEXT PIONEER_Q38_QGW8_PROGRAMS_INSPECTION PIONEER_Q38_QGW8_LAYOUT_INSPECTION
PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION
PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT TT_PIONEER_DRIVER_RELOAD'''.split()
flags=['-D'+x+'=1' for x in required]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL']
role=root/'PioneerQ38Qgw8TraceNativeProgramRole.h'
def compile_role(opts,expected):
    result=subprocess.run([compiler,'-std=c++17',*opts,'-fsyntax-only','-x','c++',str(role)],capture_output=True)
    assert (result.returncode==0)==expected,result.stderr.decode(errors='replace')[:1000]
compile_role(flags,True)
for name in required[1:]:
    current='-D'+name+'=1';compile_role([x for x in flags if x!=current],False)
    compile_role([x for x in flags if x!=current]+['-D'+name+'=0'],False)
for name in '''PIONEER_Q38_QGW8_LAYER_TRACE_PROGRAMS_INSPECTION PIONEER_Q38_QGW8_TRACE_READ_INSPECTION
PIONEER_Q38_QGW8_HIDDEN_NATIVE_129 PIONEER_Q38_HYBRID_NATIVE_COMPILE_ONLY PIONEER_Q38_QGW8_CANDIDATE_CLIENT_128
PIONEER_Q38_QGW8_TRACE_INIT_INSPECTION PIONEER_Q38_QGW8_TRACE_PACKET_INSPECTION PIONEER_Q38_BF16_FULL_NATIVE
PIONEER_Q38_HYBRID_FIXED_126 PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_NATIVE
PIONEER_Q38_WORK_INFERENCE TT_PIONEER_Q38_WORK_MODE PIONEER_Q38_CPU_VALIDATION PIONEER_Q38_FULL_MODEL_CPU_REFERENCE'''.split():
    for value in (0,1):compile_role(flags+['-D'+name+'='+str(value)],False)
compile_role([x for x in flags if not x.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]+['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010129ULL'],False)
include='#include "'+str(role)+'"\n'
for options,suffix in ((flags,'#undef PIONEER_Q38_QGW8_TRACE_NATIVE_131\n'),
                       ([x for x in flags if x!='-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1'],
                        '#define PIONEER_Q38_QGW8_TRACE_NATIVE_131 1\n')):
    result=subprocess.run([compiler,'-std=c++17',*options,'-fsyntax-only','-x','c++','-'],
                          input=include+suffix+include,text=True,capture_output=True)
    assert result.returncode,'late native program selection accepted'
# Until the lifecycle and real control path exist, the existing reload
# protocol must explicitly refuse this identity even with all role flags.
reload=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h'
reloaded=subprocess.run([compiler,'-std=c++17',*flags,'-E','-x','c++',str(reload)],capture_output=True,text=True)
assert reloaded.returncode and 'QGW8 native candidate requires its exact tag128 reload identity' in reloaded.stderr
with tempfile.TemporaryDirectory(prefix='qgw8-trace-native-program-',dir='/private/tmp') as name:
    tmp=Path(name); outputs=[]
    protocol=tmp/'protocol'
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',str(here/'pioneer_q38_qgw8_trace_native_protocol_test.cpp'),'-o',str(protocol)],check=True)
    subprocess.run([str(protocol)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
    driver_source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
    control_source=(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeControl.inc').read_text()
    constrained='#ifdef PIONEER_Q38_QGW8_TRACE_NATIVE_131\n  || !PioneerQ38Qgw8TraceNative::ControlRequestShape(*packet)\n#endif'
    assert control_source.count(constrained)==1, 'fixed trace token-control constraint is not installed'
    dispatch='#ifdef PIONEER_Q38_QGW8_TRACE_NATIVE_131\n if(operation==TT_PCI_PROBE_Q38_QGW8_TRACE_READ)\n  return Q38Qgw8TraceControlInspection(instance,data,length);\n#endif\n'
    assert driver_source.count(dispatch)==1, 'missing or ambiguous trace dispatch'
    fixture=tmp/'dispatch.cpp'
    fixture.write_text('#include <cassert>\n#include <cstddef>\n#include <cstdint>\n'
                       '#define B_DEVICE_OP_CODES_END 0x1000\n#include "'+str(here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceNativeProtocol.h')+'"\n'
                       'static int Q38Qgw8TraceControlInspection(void*,void*,size_t){return 37;}\n'
                       'static int route(uint32_t operation,void* instance,void* data,size_t length){\n'+dispatch+
                       ' (void)operation;(void)instance;(void)data;(void)length;return 0;}\n'
                       'int main(){assert(route(TT_PCI_PROBE_Q38_QGW8_TRACE_READ,nullptr,nullptr,0)==EXPECTED_TRACE);'
                       'assert(!route(TT_PCI_PROBE_Q38_QGW8_TRACE_READ+1,nullptr,nullptr,0));}\n')
    for enabled in (False,True):
        binary=tmp/('dispatch'+str(int(enabled)))
        options=['-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1'] if enabled else []
        subprocess.run([compiler,'-std=c++17','-O1','-Wall','-Wextra','-Werror','-Wno-unused-function',*options,
                        '-DEXPECTED_TRACE='+('37' if enabled else '0'),str(fixture),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    for i,image in enumerate((trace_a,trace_b)):
        out=tmp/('native'+str(i)+'.h')
        p=subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-native',str(image),str(worker),str(out)],capture_output=True,text=True)
        assert p.returncode==0,p.stderr;outputs.append(out.read_bytes())
    assert outputs[0]==outputs[1]
    inspection=tmp/'inspect.h'
    subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-inspection',str(trace_a),str(worker),str(inspection)],check=True,capture_output=True)
    marker=b'#ifndef PIONEER_Q38_HYBRID_PROGRAMS_H'
    assert outputs[0].split(marker,1)[1]==inspection.read_bytes().split(marker,1)[1], 'emitted trace program bytes differ'
    denied=tmp/'wrong.h'
    p=subprocess.run([sys.executable,str(generator),'--qgw8-layer-trace-native',str(baseline),str(worker),str(denied)],capture_output=True)
    assert p.returncode and not denied.exists()
    flags+=['-D'+x+'=1' for x in ('PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC','PIONEER_Q38_HYBRID_TRACKED_UPLOAD',
                                  'PIONEER_Q38_TOKEN_BF16_PROGRAMS','PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC',
                                  'PIONEER_Q38_QGW8_CONTROL_INSPECTION','PIONEER_Q38_QGW8_SPAN_INSPECTION')]
    source=tmp/'native0.h'
    result=subprocess.run([compiler,'-std=c++17',*flags,'-I'+str(root),'-fsyntax-only','-x','c++',str(source)],capture_output=True)
    assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
    selected=root/'PioneerQ38TokenSelectedPrograms.h'
    def select(header,options):
        return subprocess.run([compiler,'-std=c++17',*options,'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(header)+'"',
                               '-I'+str(root),'-fsyntax-only','-x','c++',str(selected)],capture_output=True)
    selection=select(source,flags)
    assert selection.returncode==0,selection.stderr.decode(errors='replace')[:1000]
    plan=root/'PioneerQ38Qgw8TraceRuntimePlan.h'
    plan_result=subprocess.run([compiler,'-std=c++17',*flags,'-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
                                '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"','-I'+str(root),'-fsyntax-only','-x','c++',str(plan)],capture_output=True)
    assert plan_result.returncode==0,plan_result.stderr.decode(errors='replace')[:2000]
    old=tmp/'old.h'
    subprocess.run([sys.executable,str(generator),'--qgw8-native-candidate',str(baseline),str(worker),str(old)],check=True,capture_output=True)
    assert select(old,flags).returncode, 'tag131 accepted the old QTP8 program'
    oldflags=[x for x in flags if x not in ('-DPIONEER_Q38_QGW8_TRACE_NATIVE_131=1','-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010131ULL')]
    oldflags+=['-DPIONEER_DRIVER_BUILD_TAG=0x2026110700010129ULL','-DPIONEER_Q38_QGW8_HIDDEN_NATIVE_129=1']
    assert select(source,oldflags).returncode, 'tag129 accepted QTPA trace program'
    if sdk is not None:
        cross=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
        headers=sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),
                               '-idirafter',str(headers/'posix'),'-fsyntax-only','-x','c++',str(source)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"',
                               '-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                               '-fsyntax-only','-x','c++',str(selected)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:1000]
        result=subprocess.run([str(cross),'-std=gnu++17',*flags,'-DPIONEER_Q38_GDN_MUTABLE_STATE=1',
                               '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"','-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),
                               '-idirafter',str(headers/'posix'),'-fsyntax-only','-x','c++',str(plan)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:2000]
        kernel=here.parents[2]/'src/add-ons/kernel/drivers/misc'
        driver=(kernel/'pioneer_q38_hybrid_full_driver.cpp').read_text()
        parents=re.findall(r'^#define ([A-Z][A-Z0-9_]*) 1$',driver,re.M)
        ttnames='BANKS PANEL AFFINE BLOCK STREAM ROWS COHORT COHORT_SEALED COHORT_PAYLOAD DISPATCH DISPATCH_VECTOR'.split()
        more=['-D'+v+'=1' for v in parents]+['-DTT_PIONEER_EXPERIMENTAL_Q38_'+v+'=1' for v in ttnames]
        probe=tmp/'native-state.cpp'
        probe.write_text('#include "'+str(kernel/'PioneerQ38TokenNativeState.h')+'"\n'
                         'static_assert(sizeof(PioneerQ38TokenNativeOwner)>0,"actual owner must compile");\n'+r'''
static volatile uint32_t dummy;
struct CompileOnlyIO {
 template<class... T> bool TokenRuntimeScope(T&&...){return dummy!=0;}
 bool TokenMemoryReadChecked(unsigned,unsigned,uint32_t& v){v=dummy;return dummy!=0;}
 bool TokenMemoryMappingStable(unsigned){return dummy!=0;}
 bool TokenPeerStoreSelect(unsigned){return dummy!=0;}
 bool SelectRuntimeCore(unsigned){return dummy!=0;}
 bool TokenFifoReadChecked(unsigned,bool,uint32_t& v){v=dummy;return dummy!=0;}
 uint32_t ResidentNocRead(unsigned){return dummy;}
 void ResidentNocWrite(unsigned,uint32_t v){dummy=v;}
 uint32_t ResetRead(){return dummy;}
 uint32_t SourceRead(unsigned){return dummy;}
 uint32_t HostPciRead(unsigned,unsigned,unsigned){return dummy;}
 uint32_t RuntimePcRead(){return dummy;}
 uint64_t Q38CohortNow(){return dummy;}
 void Pause(){dummy++;}
 void QuarantineHostPage(){dummy++;}
 void TokenMemoryRetainHostOwners(){dummy++;}
};
static auto volatile witness=&PioneerQ38TokenNativeOwner::ReadTraceChunk<CompileOnlyIO>;
''')
        result=subprocess.run([str(cross),'-std=gnu++17','-Wno-error',*flags,*more,
                               '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"',
                               '-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                               '-fsyntax-only',str(probe)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:4000]
        obj=tmp/'native-state.o'
        result=subprocess.run([str(cross),'-std=gnu++17','-O2','-fno-exceptions','-fno-rtti','-fstack-usage',*flags,*more,
                               '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(source)+'"',
                               '-I'+str(root),'-I'+str(headers),'-I'+str(headers/'os'),'-idirafter',str(headers/'posix'),
                               '-c',str(probe),'-o',str(obj)],capture_output=True)
        assert result.returncode==0,result.stderr.decode(errors='replace')[:4000]
        nm=sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
        names=subprocess.check_output([str(nm),'--defined-only',str(obj)],text=True)
        assert 'ReadTraceChunk' in names, 'native trace reader did not instantiate'
        rows=[line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        observed=[row for row in rows if 'ReadTraceChunk' in row[0]]
        assert observed and all(len(row)==3 and row[2]=='static' and int(row[1])<=2048 for row in observed),observed
        print('PASS tag131 actual owner trace reader RISC-V compile-only object and individual frame; NO real IO, driver or device')
    assert len(re.findall(rb'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ',outputs[0]))==15
print('PASS distinct tag131 build-only QTPA identity, 15 pinned sections, reproducibility and role refusals; NO DRIVER OR DEVICE')
