/* SPDX-License-Identifier: MIT */
// DATA selection only, not an image/native/producer capability.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#if PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact separate precise-activation ROOT profile required
#endif
#endif
#ifndef PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#define PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#include "PioneerQ38PreciseWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PreciseWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=49,PioneerQ38SelectedWorkerPrecise=1};
#else
#include "PioneerQ38GlobalWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38GlobalWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=46,PioneerQ38SelectedWorkerPrecise=0};
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
static_assert(PioneerQ38SelectedWorkerPrecise==1,"late worker DATA profile change");
#else
static_assert(PioneerQ38SelectedWorkerPrecise==0,"late worker DATA profile change");
#endif
