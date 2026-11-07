/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BFP4_BAND_INPUT_H
#define PIONEER_Q38_BFP4_BAND_INPUT_H
#include <cstdint>
// CPU diagnostic input only. MatchedQ4 reproduces the activation pattern in
// pioneer_native_compact_q4_stream_export.cpp, not a model-derived prompt.
namespace PioneerQ38Bfp4BandInput {
inline uint16_t Word(unsigned input,bool matchedQ4){
 const unsigned sign=(input&3)==2?0x8000:0;
 return matchedQ4?uint16_t(0x3e80+((input%256)*19+(input/256)*11)%384+sign)
  :uint16_t(0x3f00+(input*13)%256+sign);
}
}
#endif
