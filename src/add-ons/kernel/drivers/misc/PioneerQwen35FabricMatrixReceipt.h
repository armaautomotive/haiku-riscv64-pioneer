/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_FABRIC_MATRIX_RECEIPT_H
#define PIONEER_QWEN35_FABRIC_MATRIX_RECEIPT_H

#include "../../../../../tools/pioneer/PioneerQwen35DataflowProjectionQualification.h"

// One-shot metadata receipt for a separately executed recovery qualification.
// The scope must bind the real session, card and retained recovery ticket,
// prove service closure and scratch restoration, and retain this object until
// finalization. This class neither launches a projection nor grants card IO.
class PioneerQ35FabricMatrixReceipt {
    const void *fContext=nullptr,*fSession=nullptr,*fHolder=nullptr;
    const void *fScratch=nullptr,*fQualification=nullptr,*fProgram=nullptr;
    uint64_t fTicket=0;
    bool fAttempted=false,fSealed=false;
public:
    PioneerQ35FabricMatrixReceipt()=default;
    PioneerQ35FabricMatrixReceipt(const PioneerQ35FabricMatrixReceipt&)=delete;
    PioneerQ35FabricMatrixReceipt& operator=(const PioneerQ35FabricMatrixReceipt&)=delete;

    template<class Context,class Session,class Holder,class Guard,class Scope>
    bool Seal(Context& context,Session& session,Holder& holder,uint64_t ticket,Guard* guard,Scope& scope) {
        if (fAttempted) return false;
        fAttempted=true;
        auto* scratch=holder.Scratch();auto* qualification=holder.Qualification();
        if (!scratch || !qualification || !ticket || !guard || !guard->validate) return false;
        const auto* program=qualification->Program();
        using Fixture=PioneerQwen35::DataflowProjectionQualification;
        auto facts=[&]() {
            if (!holder.Active() || holder.Retained() || !holder.Owns(*scratch)
                || holder.Scratch()!=scratch || holder.Qualification()!=qualification
                || !scratch->IsRestored() || !scratch->OutputVerified()
                || !qualification->IsAudited() || qualification->Program()!=program
                || !program || !program->Valid() || program->type!=12
                || program->inputs!=Fixture::Inputs || program->outputRows!=Fixture::Rows
                || program->blocks!=Fixture::Blocks || program->groups!=PioneerQwen35::DataflowCoreCount
                || program->activeCores!=PioneerQwen35::DataflowCoreCount) return false;
            for (unsigned core=0;core<PioneerQwen35::DataflowCoreCount;core++) {
                const auto& assignment=program->core[core];
                if (!assignment.active || assignment.groups!=1 || assignment.segmentCount!=1
                    || assignment.weight[0].firstGroup!=core
                    || assignment.output[0].firstRow!=core*16 || assignment.output[0].rows!=16) return false;
            }
            return true;
        };
        auto allowed=[&]() {
            return guard->validate(ticket)==0 && facts()
                && scope.FabricMatrixReceiptSealAllowed(context,session,holder,*scratch,*qualification,ticket)
                && facts() && guard->validate(ticket)==0;
        };
        if (!allowed()) return false;
        fContext=&context;fSession=&session;fHolder=&holder;fScratch=scratch;
        fQualification=qualification;fProgram=program;fTicket=ticket;fSealed=true;
        if (!allowed()) { fSealed=false;return false; }
        return true;
    }

    // The identities are opaque here: the caller's scope must retain or
    // otherwise account for their lifetime and ensure the same proof remains
    // attached to this ticket. No object is dereferenced after session close.
    template<class Context,class Guard,class Scope>
    bool Matches(Context& context,uint64_t ticket,Guard* guard,Scope& scope) const {
        if (!fSealed || !ticket || ticket!=fTicket || fContext!=&context || !guard || !guard->validate)
            return false;
        return guard->validate(ticket)==0
            && scope.FabricMatrixReceiptStillOwned(context,fSession,fHolder,fScratch,fQualification,fProgram,ticket)
            && guard->validate(ticket)==0;
    }
};
#endif
