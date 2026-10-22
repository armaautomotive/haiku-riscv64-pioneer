/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_MODEL_PROTOCOL_H
#define PIONEER_Q38_FFN_MODEL_PROTOCOL_H
// Isolated tag35/model namespace8. Host must use the SAME authenticated
// source+bank FD that uploaded the canonical weights. No caller address,
// claimed digest,ready flag,input,code or launch capability is accepted.
// This initializes the FFN workspace ONLY,not recurrent state/model compute.
#define TT_PCI_PROBE_Q38_FFN_MODEL_SUPPORT (B_DEVICE_OP_CODES_END + 0x62a0)
#define TT_PCI_PROBE_Q38_FFN_MODEL_INITIALIZE (B_DEVICE_OP_CODES_END + 0x62a1)
struct tt_q38_ffn_model_support {
 uint32_t version,driver,model_namespace,layers,ffn_pages,requires_closed_span,model_inference,reserved;
};
struct tt_q38_ffn_model_initialize {
 uint32_t version,layer,reserved[2];
 uint32_t state,pages,closed,initialization_published,recovery_required,reserved_reply;
 uint64_t session,driver_generation;
};
#endif
