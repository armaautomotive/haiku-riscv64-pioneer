/* SPDX-License-Identifier: MIT */
#ifdef _KERNEL_MODE
#error Offline original-weight byte source is never driver code
#endif
#ifndef PIONEER_Q38_GLOBAL_OUTPUT_ORIGINAL_SOURCE_H
#define PIONEER_Q38_GLOBAL_OUTPUT_ORIGINAL_SOURCE_H
#include <cstdint>
// Separate TEST-only TU authenticates the complete SAME open public GGUF.
// Only raw Q6_K bytes cross this interface; no computed numeric products,
// native mappings,device access or full-model producer permission.
class GlobalOutputOriginalSource {
 struct Original;Original* original;
public:
 GlobalOutputOriginalSource();~GlobalOutputOriginalSource();
 GlobalOutputOriginalSource(const GlobalOutputOriginalSource&)=delete;
 GlobalOutputOriginalSource& operator=(const GlobalOutputOriginalSource&)=delete;
 bool Open(const char*);bool Ready()const;bool PrepareFork();
 bool Tile(unsigned firstRow,unsigned block,uint8_t* output,unsigned bytes);
};
#endif
