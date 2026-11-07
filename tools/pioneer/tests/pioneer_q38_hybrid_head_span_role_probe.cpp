/* SPDX-License-Identifier: MIT */
// Compile-time selection/role checks only; no execution or device access.
#define PIONEER_Q38_HYBRID_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#define B_DEVICE_OP_CODES_END 1000
#include "../PioneerQ38HybridSpanUpload.h"
struct Pages {};
struct IO {};
struct UnsupportedPlan {};
#ifdef PIONEER_TEST_UNSUPPORTED_PLAN
using Upload=PioneerQ38HybridSpanUpload<Pages,IO,UnsupportedPlan>;
#elif defined(PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC)
using Upload=PioneerQ38HybridSpanUpload<Pages,IO,PioneerQ38HybridHeadUploadPlan>;
#else
using Upload=PioneerQ38HybridSpanUpload<Pages,IO>;
#endif
static_assert(sizeof(Upload)>0,"instantiate diagnostic upload type");
