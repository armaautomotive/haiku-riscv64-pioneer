#!/usr/bin/env python3
"""Exercise the actual image choice and verification methods with fake IO."""

import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[3]
driver = root / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp'
binding = root / 'src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowDriverBinding.inc'
source = driver.read_text()
choice = source.split('// PIONEER_Q35_SELECTED_SERVICE_IMAGE_BEGIN\n', 1)[1].split(
    '// PIONEER_Q35_SELECTED_SERVICE_IMAGE_END', 1)[0]
source = binding.read_text()
pc = source.split('static bool\nPioneerQwen35DataflowServicePc(', 1)[1].split(
    '\nstatic bool\nPioneerQwen35DataflowQueuesBalanced', 1)[0]
matches = source.split('bool\nPioneerSessionIO::DataflowCoreServiceImageMatches(', 1)[1].split(
    '\nstatic bool\nPioneerQwen35DataflowCaptureLocalRoute', 1)[0]

program = r'''
#include <cassert>
#include <cstdint>
#define PIONEER_QWEN35_RESIDENT_SERVICE_WITNESS 1
#define PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS 1
namespace PioneerQwen35 {
constexpr unsigned DataflowCoreCount=120, DataflowServiceEntryAddress=0x72000;
constexpr unsigned DataflowServiceDispatchAddress=0xa0400;
constexpr unsigned DataflowServiceImageBootWord=0x6c06f, DataflowQualificationServiceBootWord=0x6c06f;
constexpr unsigned DataflowServiceStoppedParkAddress=0x72004, DataflowQualificationServiceStoppedParkAddress=0x72008;
constexpr uint32_t DataflowServiceTextWords[]={1,2},DataflowServiceDispatchWords[]={3};
constexpr uint32_t DataflowQualificationServiceTextWords[]={4,5,6},DataflowQualificationServiceDispatchWords[]={7,8};
struct DataflowServiceImageView {
    unsigned bootWord; const uint32_t* textWords; unsigned textWordCount;
    const uint32_t* dispatchWords; unsigned dispatchWordCount;
    bool Valid() const { return bootWord==0x6c06f && textWords && textWordCount && dispatchWords && dispatchWordCount; }
};
bool DataflowServicePcInImage(unsigned pc,unsigned t,unsigned d) {
    return (pc>=DataflowServiceEntryAddress && pc<DataflowServiceEntryAddress+t*4)
        || (pc>=DataflowServiceDispatchAddress && pc<DataflowServiceDispatchAddress+d*4);
}
}
struct Context {
    void* qwen35DataflowMemory=nullptr; void* qwen35DataflowService=nullptr;
    void* qwen35SharedRoleBinding=nullptr;
    bool qwen35DataflowImageVerified[120]={};
    @CHOICE@
};
struct PioneerSessionIO {
    Context* qwen35DataflowContext; const uint32_t* text; const uint32_t* dispatch;
    unsigned textSize,dispatchSize,pc=0x72000; uint32_t boot=0x6c06f;
    bool SelectRuntimeCore(unsigned core) { return core<120; }
    bool MapQ38Panel() { return true; }
    bool DataflowPanelMappingStable() { return true; }
    uint32_t RuntimeCodeRead(unsigned) { return boot; }
    uint32_t Q38PanelRead(unsigned a) {
        if (a>=0x72000 && a-0x72000<textSize*4) return text[(a-0x72000)/4];
        if (a>=0xa0400 && a-0xa0400<dispatchSize*4) return dispatch[(a-0xa0400)/4];
        return 0xffffffff;
    }
    void Pause() {}
    unsigned RuntimePcRead() { return pc; }
    bool DataflowCoreServiceImageMatches(unsigned core);
};
static bool PioneerQwen35DataflowServicePc(@PC@
bool PioneerSessionIO::DataflowCoreServiceImageMatches(@MATCHES@
int main() {
    using namespace PioneerQwen35;
    Context c; PioneerSessionIO io{&c,DataflowServiceTextWords,DataflowServiceDispatchWords,2,1};
    assert(c.SelectDataflowServiceImage(false));
    assert(c.DataflowSelectedServicePark()==DataflowServiceStoppedParkAddress);
    assert(PioneerSessionIODataflowServiceImageTrusted(io,0));
    assert(c.qwen35DataflowImageVerified[0]);
    c.qwen35DataflowService=&c;
    assert(!c.SelectDataflowServiceImage(true));
    assert(!c.qwen35QualificationServiceImage && c.qwen35DataflowImageVerified[0]);
    c.qwen35DataflowService=nullptr;
    c.qwen35SharedRoleBinding=&c;
    assert(!c.SelectDataflowServiceImage(true));
    c.qwen35SharedRoleBinding=nullptr;
    c.qwen35DataflowMemory=&c;
    assert(!c.SelectDataflowServiceImage(true));
    c.qwen35DataflowMemory=nullptr;
    assert(c.SelectDataflowServiceImage(true));
    assert(!c.qwen35DataflowImageVerified[0]);
    assert(c.DataflowSelectedServicePark()==DataflowQualificationServiceStoppedParkAddress);
    assert(!PioneerSessionIODataflowServiceImageTrusted(io,0));
    assert(!c.qwen35DataflowImageVerified[0]);
    io.text=DataflowQualificationServiceTextWords; io.textSize=3;
    io.dispatch=DataflowQualificationServiceDispatchWords; io.dispatchSize=2;
    assert(PioneerSessionIODataflowServiceImageTrusted(io,0));
    io.dispatch=DataflowServiceDispatchWords; io.dispatchSize=1;
    assert(!io.DataflowCoreServiceImageMatches(0));
    assert(!c.qwen35DataflowImageVerified[0]);
    assert(!PioneerSessionIODataflowServiceImageTrusted(io,0));
    io.dispatch=DataflowQualificationServiceDispatchWords; io.dispatchSize=2;
    assert(PioneerSessionIODataflowServiceImageTrusted(io,0));
    io.pc=0x72008;
    assert(PioneerQwen35DataflowServicePc(io));
    assert(c.SelectDataflowServiceImage(false));
    assert(!c.qwen35DataflowImageVerified[0]);
    assert(!PioneerQwen35DataflowServicePc(io));
    assert(!PioneerSessionIODataflowServiceImageTrusted(io,0));
}
'''.replace('@CHOICE@', choice).replace('@PC@', pc).replace('@MATCHES@', matches)

with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(program)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS actual service image selection and wrong-image refusal; SIMULATED IO ONLY')
