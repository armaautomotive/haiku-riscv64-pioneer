/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_IDENTITY_H
#define PIONEER_Q38_GDN_IDENTITY_H
#include <stdint.h>
// Private nonzero epoch handles, NOT raw llama sequence indexes (which may
// start at zero). Neither identity nor equality grants hardware authority.
struct PioneerQ38GdnIdentity {
 uint64_t session,sequence,modelEpoch,driverGeneration;
 bool operator==(const PioneerQ38GdnIdentity& b) const {
  return session==b.session && sequence==b.sequence && modelEpoch==b.modelEpoch && driverGeneration==b.driverGeneration;
 }
 bool Valid() const { return session && sequence && modelEpoch && driverGeneration; }
};
#endif
