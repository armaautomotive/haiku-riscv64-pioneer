/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputProof.h"
extern "C" bool pioneer_output_root_receipt_compile(const uint32_t* m,const uint32_t* q,const uint32_t* before,
 const uint32_t* after,const uint32_t* stats,const uint32_t* matrix,const uint32_t* command){
 return PioneerQ38ResidentOutputRootReceipt(m,q,before,after,stats,matrix,command);
}
extern "C" bool pioneer_output_peer_receipt_compile(const uint32_t* m,const uint32_t* q,unsigned w,const uint32_t* before,
 const uint32_t* after,const uint32_t* command,const uint32_t* receipt,const uint32_t* matrixBefore,const uint32_t* first,
 const uint32_t* recurrent,const uint32_t* gated,const uint32_t* clock){
 return PioneerQ38ResidentOutputPeerReceipt(m,q,w,before,after,command,receipt,matrixBefore,first,recurrent,gated,clock);
}
extern "C" bool pioneer_output_root_physical_compile(const uint32_t* status,const uint32_t* pc,uint32_t reset){
 return PioneerQ38ResidentOutputRootPhysical(status,pc,reset);
}
extern "C" bool pioneer_output_peer_physical_compile(const uint32_t* m,unsigned w,const uint32_t* source,const uint32_t* boot,
 const uint32_t* status,uint32_t doorbell,const uint32_t* pc,uint32_t reset,bool stop){
 return PioneerQ38ResidentOutputPeerPhysical(m,w,source,boot,status,doorbell,pc,reset,stop);
}
