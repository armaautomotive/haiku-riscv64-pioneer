/* SPDX-License-Identifier: MIT */
// Per-include identity binding, not a runtime capability.
#ifndef PIONEER_Q38_BF16_DEFERRED_FULL_BINDING_H
#define PIONEER_Q38_BF16_DEFERRED_FULL_BINDING_H
#ifdef PIONEER_Q38_BF16_DEFERRED_FULL
enum {PioneerQ38Bf16DeferredFullSelection=1};
#else
enum {PioneerQ38Bf16DeferredFullSelection=0};
#endif
#endif
#ifdef PIONEER_Q38_BF16_DEFERRED_FULL
static_assert(PioneerQ38Bf16DeferredFullSelection==1,"late deferred BF16 full selection");
#else
static_assert(PioneerQ38Bf16DeferredFullSelection==0,"late deferred BF16 full removal");
#endif
