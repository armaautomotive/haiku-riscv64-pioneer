/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_CONTINUE_SHAPE_H
#define PIONEER_Q38_RESIDENT_CONTINUE_SHAPE_H
#include "PioneerQ38ResidentAdvanceShape.h"
// NEW continuation geometry only. Neither this extra L1 nor the recipe
// GDDR extent belongs to any existing native owner/journal/Work selector.
enum {TT_Q38_CONTINUE_MARKER=0x31544351,TT_Q38_CONTINUE_FAULT=0x46544351,
 TT_Q38_CONTINUE_RECEIPT_MARKER=0x31504351,TT_Q38_CONTINUE_WORDS=128,
 TT_Q38_CONTINUE_ROM=0x30000,TT_Q38_CONTINUE_MANIFEST=0x60000,
 TT_Q38_CONTINUE_SHADOW=0x60200,TT_Q38_CONTINUE_RECEIPT=0x60400,
 TT_Q38_CONTINUE_RECEIPT_WORDS=2048,TT_Q38_CONTINUE_END=0x62400,
 TT_Q38_CONTINUE_RECIPE_OFFSET=4194304,
 TT_Q38_CONTINUE_RECIPE_WORDS=TT_Q38_COHORT_WORDS+TT_Q38_OUTPUT_COHORT_WORDS+TT_Q38_RESIDENT_LAYER_WORDS};
static inline bool PioneerQ38ResidentContinueShape(const uint32_t* d,const uint32_t* a,const uint32_t* old){
 if(!d || !a || !old || a[0]!=TT_Q38_ADVANCE_MARKER || a[3]>=63 || a[3]%4==3 || a[11]>=0x7fffffdf
  || d[0]!=TT_Q38_CONTINUE_MARKER || d[1]!=1 || d[2]!=TT_Q38_CONTINUE_WORDS
  || d[3]!=a[3] || d[4]!=a[4] || d[5]!=a[5] || d[6]!=a[6] || d[7]!=a[7] || d[8]!=a[15]
  || d[9]!=a[11]+1 || d[10]!=old[14] || d[11]!=TT_Q38_CONTINUE_RECIPE_WORDS*4
  || d[12]!=TT_Q38_CONTINUE_RECIPE_OFFSET || d[13]!=19 || d[14]!=119 || d[15]!=TT_Q38_ADVANCE_RECEIPT_MARKER)return false;
 for(unsigned i=16;i<40;i++)if(d[i]!=a[i] || d[i]!=old[i])return false;
 const unsigned fields[]={TT_Q38_COHORT_WORDS,TT_Q38_OUTPUT_COHORT_WORDS,TT_Q38_RESIDENT_LAYER_WORDS,5120,6144,5120};
 for(unsigned i=0;i<6;i++)if(d[40+i]!=fields[i])return false;
 for(unsigned i=46;i<TT_Q38_CONTINUE_WORDS;i++)if(d[i])return false;
 return true; // never a completed prefix, authenticated recipe or native grant
}
static inline bool PioneerQ38ResidentContinueRecipe(const uint32_t* d,const uint32_t* m,const uint32_t* q,const uint32_t* l,
 const uint32_t* old,const uint32_t* actualSequences){
 if(!d || !old || !PioneerQ38ResidentCohortShape(m) || !PioneerQ38ResidentOutputCohortShape(q,m)
  || !PioneerQ38ResidentLayerShape(l,m,q) || m[3]!=d[3] || m[4]!=d[4] || m[5]!=d[9]
  || m[7]!=d[6] || m[8]!=d[7] || m[9]!=d[8] || m[14]!=d[10] || q[11]!=d[5] || l[11]!=d[5])return false;
 for(unsigned i=16;i<40;i++)if(m[i]!=d[i])return false;
 for(unsigned i=0;i<120*16;i++)if(m[TT_Q38_COHORT_SOURCE_WORD+i]!=old[TT_Q38_COHORT_SOURCE_WORD+i])return false;
 for(unsigned w=0;w<119;w++)if(m[64+w]!=(actualSequences?actualSequences[w]:0))return false;
 return true; // actual sequence substitution is ON CARD, never caller readiness
}
static inline unsigned PioneerQ38ResidentContinueTarget(unsigned section){
 return section==0?TT_Q38_COHORT_MANIFEST:section==1?TT_Q38_OUTPUT_COHORT_MANIFEST:section==2?TT_Q38_RESIDENT_LAYER_MANIFEST:0;
}
static inline unsigned PioneerQ38ResidentContinueWords(unsigned section){
 return section==0?TT_Q38_COHORT_WORDS:section==1?TT_Q38_OUTPUT_COHORT_WORDS:section==2?TT_Q38_RESIDENT_LAYER_WORDS:0;
}
static_assert(TT_Q38_CONTINUE_ROM+0xa000<unsigned(TT_Q38_ADVANCE_MATRIX_ROM)
 && TT_Q38_CONTINUE_MANIFEST+TT_Q38_CONTINUE_WORDS*4==TT_Q38_CONTINUE_SHADOW
 && TT_Q38_CONTINUE_SHADOW+TT_Q38_CONTINUE_WORDS*4==TT_Q38_CONTINUE_RECEIPT
 && TT_Q38_CONTINUE_RECEIPT+TT_Q38_CONTINUE_RECEIPT_WORDS*4==TT_Q38_CONTINUE_END
 && TT_Q38_CONTINUE_END<0x64000 && TT_Q38_CONTINUE_RECIPE_WORDS*4==285184
 && TT_Q38_CONTINUE_RECIPE_OFFSET+TT_Q38_CONTINUE_RECIPE_WORDS*4<67108864,
 "NEW unqualified L1+GDDR recipe geometry;not old native mapping/initialization permission");
#endif
