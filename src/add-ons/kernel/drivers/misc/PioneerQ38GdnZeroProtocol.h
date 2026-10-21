/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_ZERO_PROTOCOL_H
#define PIONEER_Q38_GDN_ZERO_PROTOCOL_H
// Fixed reserved-state initialization qualification ONLY. No address,
// route,code,model pointer,readback buffer,slot selector or compute command.
// Separately gated QUALIFICATION build only,never ordinary mutable Work.
#define TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT (B_DEVICE_OP_CODES_END + 0x55e0)
#define TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE (B_DEVICE_OP_CODES_END + 0x55e1)
struct tt_q38_gdn_zero_support {
 uint32_t version,driver,zero_only,pages,max_regions,requires_closed_span,model_inference,pilot_pages;
};
struct tt_q38_gdn_zero_request {
 uint32_t version,mode,reserved[2]; // mode1=fixed1-page pilot,mode2=ALL owned state
 uint32_t state,regions,jobs,pages,closed,initialization_published,recovery_required;
};
#endif
