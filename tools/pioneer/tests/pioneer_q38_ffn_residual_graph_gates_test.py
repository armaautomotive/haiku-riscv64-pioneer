#!/usr/bin/env python3
"""QFG3 image/lease/stack/build/ABI gates;no hardware access."""
import hashlib
import importlib.util
import os
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

assert len(sys.argv) == 5
here = pathlib.Path(__file__).resolve().parent
directory, kernel = here.parent, here.parents[2]/'src/add-ons/kernel/drivers/misc'
roots, peer = [pathlib.Path(p) for p in sys.argv[1:3]], pathlib.Path(sys.argv[3])
old_root = pathlib.Path(sys.argv[4])
spec = importlib.util.spec_from_file_location('pins', directory/'generate_qwen38_ffn_residual_graph_images.py')
pins = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pins)
count = 0
def stack_frames(root):
    result = {}
    for name in ('program.su', 'gather.su'):
        for line in (root/name).read_text().splitlines():
            label, size, kind = line.split('\t')
            assert kind == 'static'
            match = re.fullmatch(r'(.*?):\d+:\d+:(.*)', label)
            assert match
            key = (match[1], match[2])
            result[key] = max(result.get(key, 0), int(size))
    return result
baseline = stack_frames(old_root)
assert max(baseline.values()) == 4992  # inherited matrix body;NO limit relaxation
for root in roots:
    assert (root/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
    for section, size, digest, _ in pins.ROOT.values():
        data = (root/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    assert ' U ' not in (root/'program.symbols').read_text()
    frames = stack_frames(root)
    assert frames and all(size <= 2048 or key in baseline and size <= baseline[key] for key, size in frames.items())
    assert max(size for key, size in frames.items() if any(n in key[1] for n in ('ResidualPacket', 'PreserveResidual', 'FinishResidual'))) == 96
    disassembly = (root/'program.disassembly').read_text()
    assert len(re.findall(r'\bfadd\.s\b', disassembly)) == 1
    assert not re.search(r'\b(?:fdiv\.[sd]|fsqrt\.[sd]|f(?:add|sub|mul)\.d|fmadd|fmsub|fnmadd|fnmsub)\b', disassembly)
    assert not re.search(r'\b(?:memcpy|memset|__div|__udiv|__float|__adddf|__muldf)\b', disassembly)
    count += 1
clean = {k: v for k, v in os.environ.items() if not k.startswith('PIONEER_')}
cxx = os.environ.get('CXX', 'c++')
role = 'PIONEER_Q38_FFN_GRAPH_RESIDUAL_ROOT'
with tempfile.TemporaryDirectory(prefix='pioneer-qfg3-gates-') as temporary:
    temp = pathlib.Path(temporary)
    def build_refuses(builder, flags, extra=()):
        global count
        result = subprocess.run(['sh', str(directory/builder), *extra, str(temp/'NO_SDK'), str(temp/'NO_OUTPUT')],
                                env={**clean, **flags}, capture_output=True, text=True, timeout=10)
        assert result.returncode == 2 and 'NO_SDK' not in result.stderr and not (temp/'NO_OUTPUT').exists(), (builder, flags, result.stderr)
        count += 1
    for builder in ('build_qwen38_banks_driver.sh', 'build_qwen38_banks_client.sh', 'build_qwen38_ffn_matrix_image.sh',
                    'build_qwen38_local_image.sh', 'build_qwen38_ffn_persistent_image.sh', 'build_qwen38_inference.sh',
                    'build_qwen38_projection_graph_image.sh', 'build_qwen38_projection_persistent_image.sh',
                    'build_qwen38_projection_matrix_image.sh'):
        for value in ('1', '01', '-1', '', 'invalid'):
            build_refuses(builder, {role: value})
    for value in ('2', '01', '-1', '', 'invalid'):
        build_refuses('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1', role: value})
    build_refuses('build_qwen38_ffn_graph_image.sh', {'PIONEER_Q38_FFN_GRAPH_ROOT': '1', role: '1', 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT': '1'})
    for key in (role, 'PIONEER_Q38_FFN_GRAPH_INPUT_ROOT', 'PIONEER_Q38_RESIDUAL_RMS_NATIVE', 'PIONEER_Q38_CPU_VALIDATION',
                'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_UNKNOWN_ROLE'):
        for value in ('1', '01', '', 'invalid'):
            build_refuses('build_qwen38_ffn_residual_graph_image.sh', {key: value})
    for part, (_, _, _, suffix) in pins.ROOT.items():
        result = subprocess.run([sys.executable, str(directory/'generate_qwen38_ffn_residual_graph_images.py'), str(roots[0]), str(peer), part],
                                capture_output=True, text=True, check=True)
        target = kernel/('PioneerQ38FfnResidualRoot'+suffix+'.h')
        expected = '*** Begin Patch\n*** Add File: src/add-ons/kernel/drivers/misc/'+target.name+'\n'
        expected += ''.join('+'+line+'\n' for line in target.read_text().splitlines())+'*** End Patch\n'
        assert result.stdout == expected
        count += 1
    # ALL root+peer bytes must authenticate BEFORE emitting ANY patch.
    for kind, source in (('root', roots[0]), ('peer', peer)):
        sections = ['boot', *([v[0] for v in pins.ROOT.values()] if kind == 'root' else list(pins.PEER))]
        for section in sections:
            for end in (0, -1):
                bad = temp/(kind+section+str(end));bad.mkdir()
                for name in sections:
                    data = bytearray((source/(name+'.bin')).read_bytes())
                    if name == section:
                        data[end] ^= 1
                    (bad/(name+'.bin')).write_bytes(data)
                result = subprocess.run([sys.executable, str(directory/'generate_qwen38_ffn_residual_graph_images.py'),
                                         str(bad if kind == 'root' else roots[0]), str(peer if kind == 'root' else bad), 'main'],
                                        capture_output=True, text=True)
                assert result.returncode and not result.stdout
                count += 1
    code = '#include <cassert>\n#include <fstream>\n#include <vector>\n'
    code += '#include "'+str(kernel/'PioneerQ38FfnResidualGraphImages.h')+'"\n'
    code += '#include "'+str(kernel/'PioneerQ38ProjectionGraphFootprint.h')+'"\n'
    code += '#include "'+str(here/'pioneer_q38_ffn_graph_fixture.h')+'"\n'
    code += r'''
std::vector<uint32_t> Read(const char* p){std::ifstream f(p,std::ios::binary);assert(f);std::vector<uint32_t> r;uint32_t v;while(f.read(reinterpret_cast<char*>(&v),4))r.push_back(v);return r;}
int main(int argc,char** argv){assert(argc==7);std::vector<uint32_t> a[6];for(unsigned i=0;i<6;i++)a[i]=Read(argv[i+1]);
unsigned words=0;
for(unsigned w=0;w<120;w++){
 for(unsigned i=0;i<1+(w?3072:10240)+4096;i++){
  unsigned prefix=w?3072:10240,at=!i?0x6000:i<=prefix?0x64000+(i-1)*4:0x70000+(i-prefix-1)*4;
  uint32_t expected=0;
  if(at==0x6000)expected=0x0006c06f;
  else if(at>=0x64000 && at<0x64000+a[w?5:1].size()*4)expected=a[w?5:1][(at-0x64000)/4];
  else if(!w && at>=0x68000 && at<0x68000+a[2].size()*4)expected=a[2][(at-0x68000)/4];
  else if(w && at>=0x70b00 && at<0x70b00+a[4].size()*4)expected=a[4][(at-0x70b00)/4];
  else if(at>=0x72000 && at<0x72000+a[w?3:0].size()*4)expected=a[w?3:0][(at-0x72000)/4];
  assert(PioneerQ38FfnResidualProgramWord(w,at)==expected && PioneerQ38ProjectionGraphSnapshotContains(w,at));words++;
 }
 assert(PioneerQ38FfnResidualProgramWord(w,0x6e000)==UINT32_MAX && !PioneerQ38FfnResidualExecutableAddress(w,0x6e000));
}
for(unsigned at=0x94000;at<0x9e000;at+=4)assert(PioneerQ38ProjectionGraphSnapshotContains(0,at));
assert(words==867448 && PioneerQ38FfnResidualProgramWord(120,0x6000)==UINT32_MAX);
for(unsigned bank=0;bank<7;bank++)for(unsigned layer=0;layer<64;layer++){
 FfnGraphFixture data(bank,(bank&1)!=0,true,(bank&2)!=0,bank);data.manifest[42]=layer;
 assert(PioneerQ38FfnGraphManifest(data.manifest.data(),data.source.data()));
 for(unsigned old:{1U,2U}){auto changed=data.manifest;changed[0]=old==1?0x31474651:0x32474651;changed[1]=old;
  assert(!PioneerQ38FfnGraphManifest(changed.data(),data.source.data()));}
}
}
'''
    source, binary = temp/'words.cpp', temp/'words'
    source.write_text(code)
    subprocess.run([cxx, '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror', '-D'+role+'=1', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary), *[str(roots[0]/(name+'.bin')) for name in ('text', 'prefix', 'graph')],
                    *[str(peer/(name+'.bin')) for name in ('text', 'affine', 'ffn')]], check=True)
    count += 1
    source_gate = (directory/'blackhole_qwen38_ffn_graph.cpp').read_text().split('#define block_main', 1)[0]
    def preprocess(flags, expected):
        global count
        result = subprocess.run([cxx, '-x', 'c++', '-E', *['-D'+k+'='+v for k, v in flags.items()], '-'], input=source_gate,
                                env=clean, text=True, capture_output=True, timeout=5)
        assert (result.returncode == 0) == expected, (flags, result.stderr)
        count += 1
    valid = {role: '1', 'PIONEER_Q38_FFN_GRAPH_ROOT': '1'}
    preprocess(valid, True)
    for value in ('0', '2', '-1'):
        preprocess({**valid, role: value}, False)
    for key in ('PIONEER_Q38_FFN_GRAPH_INPUT_ROOT', '_KERNEL_MODE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
                'PIONEER_Q38_RESIDENT_DECODER_NATIVE', 'PIONEER_Q38_RESIDUAL_RMS_NATIVE', 'PIONEER_Q38_FFN_PERSISTENT_GRAPH'):
        for value in ('0', '1'):
            preprocess({**valid, key: value}, False)
print(f'PASS QFG3 gates{count}/two exact images/867448codewords INSIDE original decoder snapshot/448bank-layer DATA shapes/oldABIs refuse/no CPU staging or native/Work selector/new frame<=2048B')
