/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_ABORT_H
#define PIONEER_Q38_TOKEN_BOUNDARY_ABORT_H
// Fixed diagnostic reason strings remain alive across the caller's Abort.
// Capture the FIRST cause before Abort can set its generic "aborted" reason.
// No operation/cleanup authority is derived from the returned string.
template<class Session> const char* PioneerQ38TokenBoundaryAbortReason(Session& session,const char* observation){
 const char* reason=session.failure?session.failure:observation?observation:"unknown";
 session.Abort();return reason;
} // exactly the caller's existing Abort;no retry,implicit END or device IO here
#endif
