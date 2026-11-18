/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_NATIVE_Q8_WIRE_H
#define PIONEER_Q38_TWIN_NATIVE_Q8_WIRE_H

// The opt-in QGW3 extension describes a whole cohort whose Q8 projection
// tiles use signed-magnitude quants. Only the matched authenticated loader
// may populate that cohort. Type 8 alone cannot distinguish the layouts.
namespace PioneerQ38TwinNativeQ8Wire {
enum : unsigned {
 Q8=8,Tile=576,Raw=1,Native=2,
 BootIndex=55,BootEnd=56,BootTag=0x384d5351, // QSM8
 ProofIndex=48,ProofTag=0x38505351 // QSP8
};
static constexpr unsigned ProjectionMode(unsigned type){return type==Q8?Native:Raw;}
static constexpr unsigned ProofWord(unsigned mode){return mode==Native?ProofTag:0U;}

template<class Word> static inline bool ProjectionShape(const Word* j){
 if(!j || j[26]!=ProjectionMode(j[17]))return false;
 if(j[17]!=Q8)return true; // the existing grammar still owns every other type
 return j[16]==0x31483851 && j[18]==Tile && !j[19] && !j[20]
  && (j[21]==5120 || j[21]==6144 || j[21]==17408) && j[23] && j[23]<=16 && j[25]==1;
}
}
#endif
