/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_GRAPH_PROTOCOL_H
#define PIONEER_Q38_FFN_GRAPH_PROTOCOL_H
// Exact isolated tag36/namespace9 qualification. No user code,addresses,
// geometry,source receipt,nonce or ownership/ready flags are accepted.
// One call keeps119 peers alive across up to TWO complete FFNs,then owes
// explicit graph CLOSE before BANK_END. Not an Assistant/model target.
#define TT_PCI_PROBE_Q38_FFN_GRAPH_SUPPORT (B_DEVICE_OP_CODES_END + 0x62b0)
#define TT_PCI_PROBE_Q38_FFN_GRAPH_RUN (B_DEVICE_OP_CODES_END + 0x62b1)
struct tt_q38_ffn_graph_support {
 uint32_t version,driver,model_namespace,peers,inputs,outputs,max_runs,model_inference;
};
struct tt_q38_ffn_graph_run {
 uint32_t version,layer,runs,reserved;
 uint32_t state,closed,recovery_required,reserved_reply;
 uint64_t session,driver_generation,elapsed_us[2];
 uint32_t installed_words,restored_words,peer_boots,root_starts;
 uint32_t input[2][5120],output[2][5120];
};
static_assert(sizeof(tt_q38_ffn_graph_run)>81920 && sizeof(tt_q38_ffn_graph_run)<83000,"native graph request is HEAP ONLY");
static inline bool PioneerQ38FfnGraphRequest(const tt_q38_ffn_graph_run& r)
{
 if(r.version!=1 || r.layer>=64 || r.runs<1 || r.runs>2 || r.reserved || r.state || r.closed || r.recovery_required
  || r.reserved_reply || r.session || r.driver_generation || r.elapsed_us[0] || r.elapsed_us[1]
  || r.installed_words || r.restored_words || r.peer_boots || r.root_starts)return false;
 for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<5120;i++)
  if(r.output[pass][i] || (pass>=r.runs?r.input[pass][i]!=0:(r.input[pass][i]&0x7f800000U)==0x7f800000U))return false;
 return true; // public DATA shape ONLY,never model/native/launch authority
}
#endif
