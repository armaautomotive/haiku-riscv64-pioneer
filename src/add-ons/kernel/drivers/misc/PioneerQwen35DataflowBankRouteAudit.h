/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BANK_ROUTE_AUDIT_H
#define PIONEER_QWEN35_DATAFLOW_BANK_ROUTE_AUDIT_H

#include "PioneerResidentBanks.h"
#include "../../../../../tools/pioneer/PioneerQwen35DataflowProgram.h"

// The resident fleet route preflight resolves page zero in every bank. It
// does not read or write that page. Preserve this exact check when a private
// diagnostic scratch claim is active elsewhere in the bank.
inline bool PioneerQwen35DataflowAuditBankRoutes(const PioneerResidentBanks& banks,
    const tt_dram_samples_result& dram,unsigned (&routes)[2][PioneerQwen35::Banks],uint32_t& failedBank)
{
    for (unsigned bank=0;bank<PioneerQwen35::Banks;bank++) {
        failedBank=bank;
        uint32_t address=0,route=0,index=0;
        if (!banks.Page(dram,bank,0,address,route,index)
            || route!=(dram.translated[bank][0]|(dram.translated[bank][1]<<6))) return false;
        routes[0][bank]=route;routes[1][bank]=route;
    }
    return true;
}
#endif
