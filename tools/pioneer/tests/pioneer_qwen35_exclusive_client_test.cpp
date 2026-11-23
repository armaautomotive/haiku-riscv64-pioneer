/* SPDX-License-Identifier: MIT */
// Exercise the actual client opening sequence without opening a device.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <cstring>

#define TT_PCI_PROBE_DEVICE "test/pioneer"
static int openCalls=0,mode=0,closed[2]{},closeCalls=0;
static int fake_open(const char* path,int flags) {
    assert(std::strcmp(path,"/dev/test/pioneer")==0);
    const int call=openCalls++;
    if (call==0) {
        assert((flags&(O_RDWR|O_CLOEXEC|O_EXCL))==(O_RDWR|O_CLOEXEC|O_EXCL));
        if (mode==0) { errno=EACCES;return -1; }
        return 10;
    }
    assert(call==1 && (flags&(O_RDWR|O_CLOEXEC|O_EXCL))==(O_RDWR|O_CLOEXEC));
    if (mode==1) return 11; // old driver ignores O_EXCL
    errno=mode==2?EACCES:EBUSY; return -1;
}
static int fake_close(int fd) { assert(closeCalls<2); closed[closeCalls++]=fd; return 0; }
static void fake_perror(const char*) {}
#define open fake_open
#define close fake_close
#define perror fake_perror
#include "pioneer_exclusive_client.inc"

int main() {
    for (mode=0;mode<4;mode++) {
        openCalls=closeCalls=0;closed[0]=closed[1]=-1;errno=0;
        const int fd=OpenExclusiveDataflowDriver();
        if (mode==0) assert(fd==-1 && openCalls==1 && !closeCalls);
        if (mode==1) assert(fd==-1 && openCalls==2 && closeCalls==2 && closed[0]==11 && closed[1]==10);
        if (mode==2) assert(fd==-1 && openCalls==2 && closeCalls==1 && closed[0]==10);
        if (mode==3) assert(fd==10 && openCalls==2 && !closeCalls);
    }
    std::puts("PASS rewrite client rejects an old nonexclusive driver and unexpected open errors; NO HARDWARE");
}
