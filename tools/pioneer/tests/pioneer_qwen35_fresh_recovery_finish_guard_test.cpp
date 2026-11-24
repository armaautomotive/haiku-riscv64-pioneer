/* SPDX-License-Identifier: MIT */
// Exercise the actual reset-finish guard with a retained fresh session.
#include <cassert>
#include <cstdio>
struct Context { void* recoveryFreshSession=nullptr;unsigned retained=0; };
static constexpr int B_DEV_NOT_READY=-9;
static int Finish(Context* context) {
    auto retain=[&](int status) { context->retained++;return status; };
#include "pioneer_q35_fresh_recovery_finish_guard.inc"
    return 0;
}
int main() {
    Context context;
    assert(Finish(&context)==0 && !context.retained);
    context.recoveryFreshSession=&context;
    assert(Finish(&context)==B_DEV_NOT_READY && context.retained==1);
    std::puts("PASS fresh recovery blocks generic reset completion; SIMULATED ONLY");
}
