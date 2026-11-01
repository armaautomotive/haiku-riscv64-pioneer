/* SPDX-License-Identifier: MIT */
// Bounded6607+6608 diagnostic BUILD. Whole module SHA,not tag75,is identity.
#if !defined(PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_PREFETCH_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS) || PIONEER_Q38_TOKEN_PREFETCH_PROGRAMS != 1 \
 || !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE) || PIONEER_Q38_TOKEN_FLAT_MATRIX_BENCHMARK_NATIVE != 1 \
 || !defined(PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS) || PIONEER_Q38_TOKEN_FLAT_MATRIX_PROGRAMS != 1 \
 || defined(PIONEER_Q38_TOKEN_PREFETCH_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PREFETCH_WORKER_IMAGE) \
 || defined(PIONEER_Q38_WORK_TILE_PREFETCH) || defined(PIONEER_Q38_FLAT_MATRIX_GRAMMAR_IMAGE)
#error Exact prefetch diagnostic required,not private NODRV,DATA or shader BUILD
#endif
#include "PioneerQ38TokenFlatMatrixBenchmarkRole.h"
#ifndef PIONEER_Q38_TOKEN_PREFETCH_PHYSICAL_BINDING_H
#define PIONEER_Q38_TOKEN_PREFETCH_PHYSICAL_BINDING_H
enum {PioneerQ38TokenPrefetchPhysicalSelection=1};
#endif
static_assert(PioneerQ38TokenPrefetchPhysicalSelection==1,"late prefetch diagnostic binding");
