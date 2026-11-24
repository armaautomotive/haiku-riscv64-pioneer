/* SPDX-License-Identifier: MIT */
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cassert>
#include <cstdarg>
#include <cstdio>

static unsigned opens=0,closes=0,scenario=0;
static int TestOpen(const char*,int flags,...) {
    ++opens;
    if (opens==1) {
        assert((flags & O_EXCL)!=0);
        if (scenario==1) { errno=EIO;return -1; }
        return 7;
    }
    assert((flags & O_EXCL)==0);
    if (scenario==2) return 8;
    errno=scenario==3?EIO:EBUSY;
    return -1;
}
static int TestClose(int fd) { assert(fd==7 || fd==8);++closes;return 0; }
static int TestIoctl(int,unsigned long,...) { return 0; }
#define open TestOpen
#define close TestClose
#define ioctl TestIoctl
#define main SpanQualificationMain
#include "../pioneer_qwen35_span_qualification.cpp"
#undef main
#undef ioctl
#undef close
#undef open

int main() {
    Transport io;
    for (scenario=0;scenario<4;scenario++) {
        opens=closes=0;int fd=io.Open();
        if (scenario==0) { assert(fd==7 && opens==2 && closes==0);TestClose(fd); }
        else { assert(fd==-1);assert(opens==(scenario==1?1U:2U));assert(closes==(scenario==1?0U:scenario==2?2U:1U)); }
    }
    std::puts("PASS actual Q35 qualification client exclusive open and second-open refusal; SIMULATED ONLY");
}
