/* SPDX-License-Identifier: MIT */
// DATA selection only, not an image/native/producer capability.
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#if PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION != 1 || !defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) \
 || PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1 || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#error Distinct QGW3 ROOT selection requires precise BUILD,not fixed QTP2 DATA
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION
#if PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION != 1
#error Exact separate precise-activation ROOT profile required
#endif
#endif
#if defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) && PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1
#error Exact separate QTP2 program DATA profile required
#endif
#ifndef PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#define PIONEER_Q38_SELECTED_GLOBAL_WORKER_H
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
#include "PioneerQ38PushedWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PushedWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=55,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=1};
#elif defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
#include "PioneerQ38PreciseWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38PreciseWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=49,PioneerQ38SelectedWorkerPrecise=1,PioneerQ38SelectedWorkerPushed=0};
#else
#include "PioneerQ38GlobalWorkerLayout.h"
namespace PioneerQ38SelectedGlobalWorker=PioneerQ38GlobalWorkerLayout;
enum {PioneerQ38SelectedWorkerBootEnd=46,PioneerQ38SelectedWorkerPrecise=0,PioneerQ38SelectedWorkerPushed=0};
#endif
#endif
#ifdef PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION
static_assert(PioneerQ38SelectedWorkerPushed==1,"late QGW3 DATA selection");
#else
static_assert(PioneerQ38SelectedWorkerPushed==0,"late old worker DATA selection");
#endif
#if defined(PIONEER_Q38_GLOBAL_CHAIN_PRECISE_ACTIVATION) || defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS)
static_assert(PioneerQ38SelectedWorkerPrecise==1,"late worker DATA profile change");
#else
static_assert(PioneerQ38SelectedWorkerPrecise==0,"late worker DATA profile change");
#endif
