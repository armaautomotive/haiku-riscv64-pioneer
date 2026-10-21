/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_GDN_ROOT_PROTOCOL_H
#define PIONEER_Q38_RESIDENT_GDN_ROOT_PROTOCOL_H
// Separate qualification-only protocol,NEVER ordinary model/Work. Fixed head0/root0
// diagnostic,DATA ONLY:no code,address,bank,route,slot,layer or model selector.
// CPU numerical reference belongs ONLY in a separate debug client.
#define TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT (B_DEVICE_OP_CODES_END + 0x55f0)
#define TT_PCI_PROBE_Q38_RGDN_ROOT_RUN (B_DEVICE_OP_CODES_END + 0x55f1)
#define TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT (B_DEVICE_OP_CODES_END + 0x55f2)
#define TT_PCI_PROBE_Q38_RGDN_ROOT_END (B_DEVICE_OP_CODES_END + 0x55f3)
struct tt_q38_rgdn_root_support {
 uint32_t version,driver,fixed_root,fixed_head,keys,values,requires_initialization,slots,accept_required,model_inference;
};
struct tt_q38_rgdn_root_result {
 uint32_t version,state,started,completed,held,recovery_required,nonce,awaiting,read_slot,write_slot;
 uint32_t phase,count,marker,pc[2],before[8],after[8],summary[32];
 uint64_t position;
};
struct tt_q38_rgdn_root_request {
 uint32_t version,reserved[3],decay,beta,query[128],key[128],value[64];
 tt_q38_rgdn_root_result result;
 uint32_t next[8192],output[64]; // export of GPU-verified GDDR writeback,NOT a state input
};
struct tt_q38_rgdn_root_accept { uint32_t version,nonce,reserved[2]; };
struct tt_q38_rgdn_root_end { uint32_t version,reserved,closed,recovery_required; };
static inline bool PioneerQ38RgdnRootNormal(uint32_t w) {
 return (w&0x7f800000)!=0x7f800000 && (!(w&0x7fffffff) || (w&0x7f800000));
}
static inline bool PioneerQ38RgdnRootValid(const tt_q38_rgdn_root_request& r) {
 if (r.version!=1 || r.reserved[0] || r.reserved[1] || r.reserved[2]
  || (r.decay&0x80000000) || r.decay>0x3f800000 || !PioneerQ38RgdnRootNormal(r.decay)
  || (r.beta&0x80000000) || r.beta>0x3f800000 || !PioneerQ38RgdnRootNormal(r.beta)) return false;
 for (auto w:r.query) if (!PioneerQ38RgdnRootNormal(w)) return false;
 for (auto w:r.key) if (!PioneerQ38RgdnRootNormal(w)) return false;
 for (auto w:r.value) if (!PioneerQ38RgdnRootNormal(w)) return false;
 return true;
}
#endif
