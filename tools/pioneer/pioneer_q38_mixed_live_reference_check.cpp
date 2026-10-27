/* SPDX-License-Identifier: MIT */
// CPU-only developer diagnostic. Opens the fixed authenticated original
// model file, NEVER a P100A/driver/accelerator context or Work backend.
#include "PioneerQ38MixedLiveOriginalSource.h"
#include <cstdio>
int main(int argc,char**){
 if(argc!=1)return 2;
 setvbuf(stdout,nullptr,_IOLBF,0);
 puts("QBL1_CPU_REFERENCE_ONLY authenticating fixed original model;NO accelerator context");
 auto source=OpenPioneerQ38MixedLiveOriginalSource();
 if(!source){std::fputs("QBL1_CPU_REFERENCE_ONLY original authentication failed\n",stderr);return 1;}
 if(!source->Prepare() || !source->SealBeforeNativeContext() || !source->Ready()){
  std::fputs("QBL1_CPU_REFERENCE_ONLY reference preparation/seal failed\n",stderr);return 1;
 }
 printf("QBL1_CPU_REFERENCE_READY reads%llu fingerprint%016llx;standalone layer3/position0/from original embedding;NOT GDN0..2/fullmodel/cardcomparison/Work/TPS\n",
  (unsigned long long)source->ReferenceReads(),(unsigned long long)source->ReferenceFingerprint());
 return 0;
}
