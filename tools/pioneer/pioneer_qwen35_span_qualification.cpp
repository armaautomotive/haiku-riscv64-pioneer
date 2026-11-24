/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include "PioneerQwen35SpanQualificationClient.h"
struct Transport {
    int Open() { return open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC); }
    bool Call(int fd,uint32_t op,void* data,size_t size) { return ioctl(fd,op,data,size)==0; }
    int Close(int fd) { return close(fd); }
};
int main(int argc,char** argv)
{
    if (argc!=2 || strcmp(argv[1],"--qualify-supported-span")) return 2;
    Transport io;
    if (!PioneerQwen35SpanQualificationClient(io)) return 1;
    puts("P100A_Q35_SUPPORTED_SPAN banks=7 pages=7 host_exact=1 clean_end=1;NO_INFERENCE");
    return 0;
}
