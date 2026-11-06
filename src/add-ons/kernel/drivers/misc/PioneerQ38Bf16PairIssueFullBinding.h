/* SPDX-License-Identifier: MIT */
// Per-include binding. Never silently add/remove paired inference identity.
#ifndef PIONEER_Q38_BF16_PAIR_ISSUE_FULL_BINDING_H
#define PIONEER_Q38_BF16_PAIR_ISSUE_FULL_BINDING_H
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
enum {PioneerQ38Bf16PairIssueFullSelection=1};
#else
enum {PioneerQ38Bf16PairIssueFullSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_BF16_PAIR_ISSUE_FULL
static_assert(PioneerQ38Bf16PairIssueFullSelection==1,"late paired BF16 full selection");
#else
static_assert(PioneerQ38Bf16PairIssueFullSelection==0,"late paired BF16 full removal");
#endif
