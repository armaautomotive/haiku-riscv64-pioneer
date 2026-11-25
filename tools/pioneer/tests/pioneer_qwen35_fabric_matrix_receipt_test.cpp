/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35FabricMatrixReceipt.h"
#include <cassert>
#include <cstdio>
#include <memory>
using namespace PioneerQwen35;
using Fixture=DataflowProjectionQualification;
static unsigned checks=0,loseAt=0;
static int Validate(uint64_t ticket) { ++checks;return ticket==7 && checks!=loseAt?0:-1; }
struct Guard { int (*validate)(uint64_t)=Validate; };
struct FakeScratch {
    bool restored=true,output=true;
    bool IsRestored() const { return restored; }
    bool OutputVerified() const { return output; }
};
struct FakeQualification {
    DataflowProjectionProgram* program=nullptr;bool audited=true;
    bool IsAudited() const { return audited; }
    const DataflowProjectionProgram* Program() const { return program; }
};
struct Holder {
    FakeScratch scratch;FakeQualification qualification;bool active=true,retained=false;
    FakeScratch* Scratch() { return &scratch; }
    FakeQualification* Qualification() { return &qualification; }
    bool Active() const { return active; }
    bool Retained() const { return retained; }
    bool Owns(const FakeScratch& candidate) const { return &candidate==&scratch; }
};
struct Scope {
    bool seal=true,owned=true;
    const void *session=nullptr,*holder=nullptr,*scratch=nullptr,*qualification=nullptr,*program=nullptr;
    template<class C,class S,class H,class A,class Q>
    bool FabricMatrixReceiptSealAllowed(C&,S& s,H& h,A& a,Q& q,uint64_t ticket) {
        return seal && ticket==7 && &s==session && &h==holder && &a==scratch && &q==qualification;
    }
    template<class C>
    bool FabricMatrixReceiptStillOwned(C&,const void* s,const void* h,const void* a,
        const void* q,const void* p,uint64_t ticket) {
        return owned && ticket==7 && s==session && h==holder && a==scratch && q==qualification && p==program;
    }
};
struct FixtureState {
    int context=1,session=2;Holder holder;Guard guard;Scope scope;
    std::unique_ptr<DataflowProjectionProgram> program=std::make_unique<DataflowProjectionProgram>();
    std::unique_ptr<PioneerQ35ProjectionProgram> schedule=std::make_unique<PioneerQ35ProjectionProgram>();
    FixtureState() {
        unsigned routes[2][Banks]{};
        for (unsigned n=0;n<2;n++) for (unsigned b=0;b<Banks;b++) routes[n][b]=b+1;
        assert(Fixture::Build(0,BankBase,Fixture::ScratchBytes,routes,*program,*schedule));
        holder.qualification.program=program.get();
        scope.session=&session;scope.holder=&holder;scope.scratch=&holder.scratch;
        scope.qualification=&holder.qualification;scope.program=program.get();
        checks=loseAt=0;
    }
    bool Seal(PioneerQ35FabricMatrixReceipt& receipt) {
        return receipt.Seal(context,session,holder,7,&guard,scope);
    }
};
int main()
{
    {
        FixtureState f;PioneerQ35FabricMatrixReceipt receipt;
        assert(f.Seal(receipt) && receipt.Matches(f.context,7,&f.guard,f.scope));
        assert(!f.Seal(receipt) && !receipt.Matches(f.context,8,&f.guard,f.scope));
        f.scope.owned=false;assert(!receipt.Matches(f.context,7,&f.guard,f.scope));
        f.scope.owned=true;loseAt=checks+1;assert(!receipt.Matches(f.context,7,&f.guard,f.scope));
    }
    for (unsigned kind=0;kind<8;kind++) {
        FixtureState f;PioneerQ35FabricMatrixReceipt receipt;
        switch (kind) {
        case 0: f.holder.active=false;break;
        case 1: f.holder.retained=true;break;
        case 2: f.holder.scratch.restored=false;break;
        case 3: f.holder.scratch.output=false;break;
        case 4: f.holder.qualification.audited=false;break;
        case 5: f.program->core[119].active=0;break;
        case 6: f.scope.seal=false;break;
        case 7: loseAt=2;break;
        }
        assert(!f.Seal(receipt));
        f.holder.active=true;f.holder.retained=false;f.holder.scratch.restored=true;
        f.holder.scratch.output=true;f.holder.qualification.audited=true;f.scope.seal=true;loseAt=0;
        assert(!f.Seal(receipt) && !receipt.Matches(f.context,7,&f.guard,f.scope));
    }
    for (unsigned failure=1;failure<=4;failure++) {
        FixtureState f;PioneerQ35FabricMatrixReceipt receipt;loseAt=failure;
        assert(!f.Seal(receipt));
        loseAt=0;assert(!receipt.Matches(f.context,7,&f.guard,f.scope) && !f.Seal(receipt));
    }
    std::puts("PASS recovery matrix receipt requires audited full fleet, exact output and restored scratch; SIMULATED ONLY");
}
