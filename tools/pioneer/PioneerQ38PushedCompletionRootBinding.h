/* SPDX-License-Identifier: MIT */
// Rechecked even after a previous include bound the opposite BUILD profile.
#ifndef PIONEER_Q38_PUSHED_COMPLETION_ROOT_BINDING_H
#define PIONEER_Q38_PUSHED_COMPLETION_ROOT_BINDING_H
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
enum {PioneerQ38PushedCompletionRootProfile=1};
#else
enum {PioneerQ38PushedCompletionRootProfile=0};
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
static_assert(PioneerQ38PushedCompletionRootProfile==1,"late pushed completion ROOT policy");
#else
static_assert(PioneerQ38PushedCompletionRootProfile==0,"late old completion ROOT policy");
#endif
