/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
using status_t=int;
enum { B_OK=0,B_NOT_ALLOWED=-2,B_ERROR=-3 };
static const char* TT_PCI_PROBE_DEVICE="p100a-test";
static char sCookie;
struct Card { unsigned bus=1,device=2,function=3; } sCard;
struct PioneerStoreContext { bool q35PciReservationHeld=false,q35PciReleaseAttempted=false; };
struct FakePci {
    PioneerStoreContext* context=nullptr;unsigned calls=0;status_t result=B_OK;
    status_t unreserve_device(unsigned bus,unsigned device,unsigned function,const char* name,void* cookie) {
        assert(bus==1 && device==2 && function==3 && name==TT_PCI_PROBE_DEVICE && cookie==&sCookie);
        assert(context && !context->q35PciReservationHeld && context->q35PciReleaseAttempted);
        ++calls;return result;
    }
} pci,*sPCI=&pci;
#include "q35_pci_reservation_release.inc"
int main()
{
    {
        PioneerStoreContext context;pci={&context,0,B_OK};
        assert(Qwen35ReleasePciReservation(context)==B_NOT_ALLOWED && !pci.calls);
        context.q35PciReservationHeld=true;
        assert(Qwen35ReleasePciReservation(context)==B_OK && pci.calls==1);
        assert(!context.q35PciReservationHeld && context.q35PciReleaseAttempted
            && Qwen35ReleasePciReservation(context)==B_NOT_ALLOWED && pci.calls==1);
    }
    {
        PioneerStoreContext context;context.q35PciReservationHeld=true;pci={&context,0,B_ERROR};
        assert(Qwen35ReleasePciReservation(context)==B_ERROR && pci.calls==1
            && !context.q35PciReservationHeld && context.q35PciReleaseAttempted);
        pci.result=B_OK;
        assert(Qwen35ReleasePciReservation(context)==B_NOT_ALLOWED && pci.calls==1);
    }
    puts("PASS retained PCI reservation release: ownership invalidated before one-shot unreserve; SIMULATED ONLY");
}
