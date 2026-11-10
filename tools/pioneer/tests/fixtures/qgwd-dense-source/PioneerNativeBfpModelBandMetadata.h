/* Synthetic metadata for CPU source role/invalid-input tests only. */
#ifndef PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H
#define PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H
#include <stdint.h>
struct PioneerNativeBfpModelBandMetadata {
 static constexpr unsigned Width=5120,Rows=1024,Tensor=45,Samples=3;
 static unsigned First(unsigned sample){const unsigned rows[3]={0,512,992};return sample<3?rows[sample]:~0U;}
 static const uint32_t* Golden(unsigned){static const uint32_t words[32]={};return words;}
 static const char* PayloadSHA(unsigned){return "synthetic-not-a-sha";}
};
#endif
