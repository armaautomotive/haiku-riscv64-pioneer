/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_CAPSULE_HOOK_H
#define PIONEER_Q38_MIXED_CAPSULE_HOOK_H
#ifdef PIONEER_Q38_MIXED_CAPSULE_IMAGE
#if PIONEER_Q38_MIXED_CAPSULE_IMAGE != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(__FAST_MATH__)
#error Internal OFFLINE capsule only, not native/CPU/Work
#endif
// ALL capsules reserve s11. Only the NEW front end sets it to its live
// private stack snapshot. No mutable global/mailbox manufactures a seal.
extern "C" void q38_mixed_seal();
#define PIONEER_Q38_MIXED_SEAL() q38_mixed_seal()
#else
#define PIONEER_Q38_MIXED_SEAL() ((void)0)
#endif
#endif
