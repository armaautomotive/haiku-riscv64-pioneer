/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_HAIKU_QWEN38_H
#define TENSTORRENT_HAIKU_QWEN38_H
#ifndef TT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED
#error This separate host session requires explicit boundary-sealed cohort compilation,not a legacy live upgrade
#endif
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "PioneerQ38CohortSession.h"
class PioneerQ38NativeTransport {
public:
	int Open() { return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC); }
	bool Call(int fd,uint32_t op,void* data,size_t bytes) { return !ioctl(fd,op,data,bytes); }
	int Close(int fd) { return close(fd); }
	uint64_t NowNanos() { timespec t={}; return clock_gettime(CLOCK_MONOTONIC,&t)?0:uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec); }
};
using PioneerQ38NativeSession=PioneerQ38CohortSession<PioneerQ38NativeTransport>;
#endif
