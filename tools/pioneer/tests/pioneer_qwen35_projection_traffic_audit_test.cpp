/* SPDX-License-Identifier: MIT */
#define main q35_projection_traffic_audit_main
#include "pioneer_qwen35_projection_traffic_audit.cpp"
#undef main
#include <cassert>
int main() {
 std::vector<Span> spans={{1,16,16},{0,32,16},{0,16,16}};
 assert(NonOverlapping(spans));
 spans.push_back({0,31,2});assert(!NonOverlapping(spans));
 spans={{0,0xfffffff0U,32},{0,0xfffffff8U,8}};
 assert(!NonOverlapping(spans));
 std::puts("PASS projection traffic span overlap checks; NO HARDWARE");
}
