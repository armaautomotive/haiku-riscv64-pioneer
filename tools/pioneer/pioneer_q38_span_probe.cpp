/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include "PioneerQ38SpanPublicTest.h"
struct Transport {
	int Open() { return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC); }
	bool Call(int fd,uint32_t op,void* data,size_t bytes) { return ioctl(fd,op,data,bytes)==0; }
	int Close(int fd) { return close(fd); }
	uint64_t NowNanos() { timespec t={}; return clock_gettime(CLOCK_MONOTONIC,&t)?0:uint64_t(t.tv_sec)*1000000000+uint64_t(t.tv_nsec); }
};
int main(int argc,char** argv)
{
	if (argc!=2) return 2;
	bool work=!strcmp(argv[1],"--public-work-span-transport");
	if (!work && strcmp(argv[1],"--public-span-transport")) return 2;
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("PUBLIC_SPAN_TRANSPORT_ONLY;NO_MODEL_OR_INFERENCE;FAILURE_CLOSE_ONLY_NO_END_RETRY_RESET");
	Transport io; return (work?PioneerQ38SpanPublicTest<Transport,true>(io):PioneerQ38SpanPublicTest(io))?0:1;
}
