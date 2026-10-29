/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_FIFO_QUIESCENCE_H
#define PIONEER_Q38_TOKEN_FIFO_QUIESCENCE_H
#include <stdint.h>
// Blackhole Request FIFOs must drain BEFORE any outstanding-counter receipt.
// NIU_CFG_0 bit16 controls FIFOs,not bit14 coordinate translation. See:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md#niu-request-fifos
// This read-order primitive grants NO source/STOP/HOLD/restore/END authority.
template<class IO> bool PioneerQ38TokenFifoEmpty(IO& io,unsigned worker,uint32_t knownConfig){
 uint32_t value=0;
 if(!io.TokenFifoReadChecked(worker,false,value) || value!=((knownConfig&0x10000U)?0x10101010U:0U))return false;
 return io.TokenFifoReadChecked(worker,true,value) && value==0;
} // checked exact four-FIFO availability FIRST,then sticky overflow;never clear or start
#endif
