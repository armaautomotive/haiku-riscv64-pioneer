/* SPDX-License-Identifier: MIT */
// Private renamed-ABI NODRV context only;NOT an installable driver or Work.
#if !defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS) || PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS) || PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_PROGRAMS != 1
#error Exact new cohort-source private context and commit-boundary parents required
#endif
#if defined(PIONEER_Q38_TOKEN_COHORT_LOCAL_SOURCES_BENCHMARK_NATIVE) \
 || defined(PIONEER_Q38_COHORT_LOCAL_SOURCES_IMAGE)
#error Cohort-source private context cannot alias diagnostic or ROOT BUILD
#endif
#include "PioneerQ38TokenCommitBoundaryNativeRole.h"
