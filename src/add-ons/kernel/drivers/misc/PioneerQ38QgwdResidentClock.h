/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_QGWD_RESIDENT_CLOCK_H
#define PIONEER_Q38_QGWD_RESIDENT_CLOCK_H
// The actual clock transition rejects a context with a published span. The
// caller owns the virgin preallocation on entry and, on failure, must retain
// it alongside the context before quarantining; this helper performs no cleanup.
template<class Context,class Span,class BeginClock,class Current>
static bool PioneerQ38QgwdResidentClockBeforeSpan(Context& context,Span*& fresh,
 BeginClock beginClock,Current current)
{
 if(!fresh || context.q38Span)return false;
 if(!beginClock(context) || !current())return false;
 context.q38Span=fresh;fresh=nullptr;return true;
}
#endif
