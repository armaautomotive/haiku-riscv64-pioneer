/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FULL_OFFLOAD_EOG_H
#define PIONEER_Q38_FULL_OFFLOAD_EOG_H
// Tokenizer metadata of the one SHA-pinned PUBLIC model,not learned weights.
// Match the frozen tokenizer's WHOLE end-of-generation set,including code
// delimiters. The full-card firmware and its record reader use the same set;
// the old two-EOS diagnostic/Work families are deliberately unchanged.
namespace PioneerQ38FullOffloadEog {
enum {Count=5,Eos0=248044,Eos1=248046};
static inline bool Contains(unsigned id){
 return id==Eos0 || id==Eos1 || (id>=248063 && id<=248065);
}
}
#endif
