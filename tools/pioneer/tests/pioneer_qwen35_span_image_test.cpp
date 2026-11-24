/* SPDX-License-Identifier: MIT */
// Interpret the actual Q35 RV32 image with simulated host, GDDR and NoC.
#define PIONEER_Q38_SPAN_MACHINE_ONLY
#include "pioneer_q38_span_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35SpanSummary.h"

struct Q35SpanMachine : Q38SpanMachine {
    unsigned missingReadResponse=0;
    Q35SpanMachine(const char* image,unsigned bytes,unsigned bank=0,bool edge=false)
        :Q38SpanMachine(image,bytes,bank,edge) {}
    uint32_t Load(uint32_t address) override {
        assert(address!=0xffb2020cU); // unsupported on Blackhole
        return Q38SpanMachine::Load(address);
    }
    void Store(uint32_t address,uint32_t value) override {
        Q38SpanMachine::Store(address,value);
        if (address==0xffb20040U && transfers==missingReadResponse && command[7]==0)
            counts[0]--; // accepted and sent, but response never observed
    }
    void VerifyQ35() {
        assert(!pending && hostData==golden && gddr==golden);
        uint32_t summary[32],before[8],after[8];
        for (unsigned i=0;i<32;i++) summary[i]=memory.at(0x143000+i*4);
        for (unsigned i=0;i<8;i++) { before[i]=summary[8+i];after[i]=summary[16+i]; }
        assert(PioneerQwen35SpanSummary(summary,descriptor.data(),before,after));
        for (unsigned i=0;i<32;i++) {
            summary[i]^=1;assert(!PioneerQwen35SpanSummary(summary,descriptor.data(),before,after));summary[i]^=1;
        }
        assert(!before[1] && !after[1]);
    }
};

int main(int argc,char** argv)
{
    assert(argc==2);
    for (unsigned bank=0;bank<7;bank++) for (bool edge:{false,true})
        for (unsigned bytes:{4096U,20480U,1048576U}) for (unsigned mode:{1U,2U}) {
            Q35SpanMachine machine(argv[1],bytes,bank,edge);
            machine.descriptor[2]=machine.memory[0x140008]=mode;
            assert(machine.Run());machine.VerifyQ35();
            assert(machine.transfers==(mode==2?2U:7U)*((bytes+16383)/16384));
        }
    for (unsigned packet:{1U,2U,128U}) for (unsigned fault=0;fault<3;fault++) {
        Q35SpanMachine machine(argv[1],1048576);
        machine.descriptor[2]=machine.memory[0x140008]=2;
        if (!fault) machine.failPacket=packet;
        else if (fault==1) machine.badCounter=packet;
        else machine.changeSourceAfter=packet;
        assert(!machine.Run() && machine.transfers==packet);
    }
    {
        Q35SpanMachine machine(argv[1],4096);
        machine.descriptor[2]=machine.memory[0x140008]=2;
        machine.missingReadResponse=1;
        assert(!machine.Run() && machine.transfers==1);
    }
    puts("PASS Q35 preload RV32: seven banks, strict/work, supported counters and retained faults; SIMULATED ONLY");
}
