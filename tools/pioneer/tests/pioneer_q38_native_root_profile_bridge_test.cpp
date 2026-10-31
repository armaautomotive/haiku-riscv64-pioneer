/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_native_root_profile_bridge.h"
int main(){
 // Synthetic PC observations ONLY. No ROM execution,ready/model products
 // or native lifetime is inferred from this isolated bridge contract test.
 Q38NativeRootProfileRetired(0x6000);
 Q38NativeRootProfileRetired(0x40000);
 Q38NativeRootProfileRetired(0x40004);
 Q38NativeRootProfileRetired(0x40000);
 Q38NativeRootProfileRetired(0x64000);
 Q38NativeRootProfileRetired(0x60004);
 Q38NativeRootProfileReport(6);
}
