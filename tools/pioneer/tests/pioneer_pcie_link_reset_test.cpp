/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPcieLinkReset.h"
struct Write {unsigned offset,width,value;};
struct Fake {
    uint32_t rootVendor=0x20421e30,rootClass=0x06040000,buses=0x003f0100;
    uint32_t rootCommand=0x402,cardCommand=0x402,cardVendor=0xb1401e52,subsystem=0x00431e52,control=3;
    unsigned checks=0,loseAt=0,delayCount=0,failedDelay=0,polls=0,readyAfter=0,endpointPolls=0;
    bool held=false,linkReset=false,ignoreAssert=false,injectDiscard=false;
    std::vector<Write> writes;std::vector<unsigned> delays;
    bool Allowed(){++checks;return !loseAt || checks<loseAt;}
    uint32_t RootRead(unsigned offset,unsigned width){
        if(offset==0&&width==4)return rootVendor;
        if(offset==8&&width==4)return rootClass;
        if(offset==0x18&&width==4)return buses;
        if(offset==4&&width==2)return rootCommand;
        if(offset==0x3e&&width==2)return control;
        assert(false);return 0;
    }
    uint32_t CardRead(unsigned offset,unsigned width){
        if(offset==0&&width==4){if(linkReset)++endpointPolls;return cardVendor;}
        if(offset==0x2c&&width==4)return subsystem;
        if(offset==4&&width==2)return cardCommand;
        assert(false);return 0;
    }
    void RootWrite(unsigned offset,unsigned width,uint32_t value){
        assert(!loseAt || checks<loseAt);assert(offset==0x3e&&width==2);
        writes.push_back({offset,width,value});
        uint32_t status=control&0x400;
        if(value&0x400)status=0; // W1C: record would be lost if replayed.
        if(value&0x40) {
            if(!ignoreAssert)held=true;
            if(injectDiscard)status=0x400;
        } else if(held){held=false;linkReset=true;}
        control=(value&~0x400U)|status;
        if(ignoreAssert)control&=~0x40U;
    }
    bool DelayUs(unsigned us){++delayCount;delays.push_back(us);return failedDelay!=delayCount;}
    bool RootLinkReady(){return ++polls>readyAfter;}
};
static void Success(){
    Fake io;io.readyAfter=2;PioneerPcieLinkReset<Fake> reset;
    assert(reset.Run(io,101));auto r=reset.Result();
    assert(r.stage==PioneerPcieLinkResetReturned && r.polls==3 && io.endpointPolls==1 && r.assertConfirmed && r.deassertConfirmed);
    assert(io.writes.size()==2 && io.writes[0].value==0x43 && io.writes[1].value==3);
    assert((io.delays==std::vector<unsigned>{2000,500000,100000,100000}));
    assert(!reset.Run(io,101) && io.writes.size()==2);
}
static void PreflightAndFailures(){
    for(unsigned variant=0;variant<7;variant++){
        Fake io;PioneerPcieLinkReset<Fake> reset;
        if(variant==0)io.cardCommand|=4;
        if(variant==1)io.rootCommand|=4;
        if(variant==2)io.rootVendor=0xffffffff;
        if(variant==3)io.cardVendor=0xffffffff;
        if(variant==4)io.control|=0x40;
        if(variant==5)io.control|=0x400;
        assert(!reset.Run(io,variant==6?0:101));assert(io.writes.empty());
    }
    for(unsigned failure=1;failure<=2;failure++){
        Fake io;io.failedDelay=failure;PioneerPcieLinkReset<Fake> reset;
        assert(!reset.Run(io,101));auto r=reset.Result();
        assert(r.assertAttempted && r.deassertAttempted && io.writes.size()==2 && !io.held);
        assert(!reset.Run(io,101) && io.writes.size()==2);
    }
    Fake ignored;ignored.ignoreAssert=true;PioneerPcieLinkReset<Fake> ignoredReset;
    assert(!ignoredReset.Run(ignored,101) && ignored.writes.size()==2 && !ignoredReset.Result().assertConfirmed
        && ignoredReset.Result().failedAt==PioneerPcieLinkResetDeasserted);
    Fake discard;discard.injectDiscard=true;PioneerPcieLinkReset<Fake> discardReset;
    assert(!discardReset.Run(discard,101) && discard.writes.size()==2 && (discard.control&0x400));
    assert(!(discard.writes[0].value&0x400) && !(discard.writes[1].value&0x400));
    Fake timeout;timeout.readyAfter=200;PioneerPcieLinkReset<Fake> timeoutReset;
    assert(!timeoutReset.Run(timeout,4) && timeoutReset.Result().polls==4 && timeout.endpointPolls==0
        && timeoutReset.Result().failedAt==PioneerPcieLinkResetWaiting && timeout.writes.size()==2);
    assert(timeout.delays.size()==5 && !timeoutReset.Run(timeout,4) && timeout.writes.size()==2);
}
static void AuthorityLoss(){
    Fake baseline;PioneerPcieLinkReset<Fake> ok;assert(ok.Run(baseline,101));
    for(unsigned point=1;point<=baseline.checks;point++){
        Fake io;io.loseAt=point;PioneerPcieLinkReset<Fake> reset;
        assert(!reset.Run(io,101));assert(io.writes.size()<=2);
        auto count=io.writes.size();assert(!reset.Run(io,101) && io.writes.size()==count);
    }
}
int main(){Success();PreflightAndFailures();AuthorityLoss();
    std::puts("PASS SG2042 link reset sequence, W1C preservation, failure deassert and one-shot ownership; SIMULATED ONLY");}
