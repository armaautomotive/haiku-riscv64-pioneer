/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FRONTEND_BARRIER_H
#define PIONEER_Q38_FRONTEND_BARRIER_H
#ifndef PIONEER_Q38_WORK_FRONTEND_BARRIER
#error Separate OFFLINE original-layout Work scheduling experiment ONLY
#endif
// SAME idle conditions1816 (unpack0/1,FPU,SFPU,configuration) as the
// qualified drain;block ALL9 frontend classes,not merely Scalar/ACK.
// ONLY between Tensix instructions. Never substitutes for a RISC-facing
// drain before CPU MMIO/config/L1/NoC access,publication or final proof.
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/STALLWAIT.md
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/Diagrams/Out/Bits32_STALLWAIT_BH.svg
template<class E> static inline void PioneerQ38FrontendBarrier(E& e,bool work) {
 if(work)e.Op(0xa2ff9816);else e.Drain();
}
#endif
