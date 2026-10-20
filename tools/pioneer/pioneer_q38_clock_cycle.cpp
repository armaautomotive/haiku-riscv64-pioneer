/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "PioneerQ38ClockCycle.h"
struct ClockCycleIO {
	int Open() { return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC); }
	bool Call(int fd,uint32_t op,void* data,size_t bytes) { return ioctl(fd,op,data,bytes)==0; }
	bool Close(int fd) { return close(fd)==0; }
};
int main(int argc,char** argv)
{
	if (argc!=2 || strcmp(argv[1],"--normal-clock-owner-cycle")) return 2;
	ClockCycleIO io; const bool okay=PioneerQ38ClockCycle(io);
	puts(okay?"PASS_NORMAL_CLOCK_OWNER_CYCLE known_clean_END1;NO_COMPUTE_OR_UPLOAD;ACTUAL_CLOCKS_IN_KERNEL_LOG"
		:"STOP_CLOCK_OWNER_CYCLE;CLOSE_ONLY_NO_END_RETRY_RESET_OR_ADDITIONAL_CARD_READ");
	return okay?0:1;
}
