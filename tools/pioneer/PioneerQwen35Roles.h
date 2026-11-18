/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ROLES_H
#define PIONEER_QWEN35_ROLES_H

struct PioneerQ35Expected {
 const char* name; unsigned inputs,outputs,type,alternate,rank;
};
static inline const PioneerQ35Expected* PioneerQ35Role(unsigned layer,unsigned role) {
 static const PioneerQ35Expected common[]={
  {"attn_norm.weight",2560,1,0,0,1},{"post_attention_norm.weight",2560,1,0,0,1},
  {"ffn_gate.weight",2560,9216,12,12,2},{"ffn_up.weight",2560,9216,12,12,2},
  {"ffn_down.weight",9216,2560,12,14,2}};
 static const PioneerQ35Expected recurrent[]={
  {"attn_gate.weight",2560,4096,12,12,2},{"attn_qkv.weight",2560,8192,13,13,2},
  {"ssm_a",32,1,0,0,1},{"ssm_dt.bias",32,1,0,0,1},{"ssm_norm.weight",128,1,0,0,1},
  {"ssm_alpha.weight",2560,32,8,8,2},{"ssm_beta.weight",2560,32,8,8,2},
  {"ssm_conv1d.weight",4,8192,0,0,2},{"ssm_out.weight",4096,2560,13,13,2}};
 static const PioneerQ35Expected attention[]={
  {"attn_q.weight",2560,8192,12,12,2},{"attn_k.weight",2560,1024,12,12,2},
  {"attn_v.weight",2560,1024,12,14,2},{"attn_q_norm.weight",256,1,0,0,1},
  {"attn_k_norm.weight",256,1,0,0,1},{"attn_output.weight",4096,2560,12,12,2}};
 static const PioneerQ35Expected special[]={
  {"token_embd.weight",2560,248320,14,14,2},{"output_norm.weight",2560,1,0,0,1}};
 if (layer==32) return role<2?&special[role]:nullptr;
 if (layer>=32) return nullptr;
 if (role<5) return &common[role];
 role-=5;
 return layer%4==3?(role<6?&attention[role]:nullptr):(role<9?&recurrent[role]:nullptr);
}

#endif
