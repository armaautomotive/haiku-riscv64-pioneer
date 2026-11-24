/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include "PioneerQwen35SpanQualificationClient.h"
struct Transport {
    int Open() {
        const char* device="/dev/misc/tenstorrent_pci_probe";
        int fd=open(device,O_RDWR|O_CLOEXEC|O_EXCL);
        if (fd<0) { perror("P100A_Q35_SUPPORTED_SPAN open_exclusive"); return -1; }
        int probe=open(device,O_RDWR|O_CLOEXEC);
        int error=errno;
        if (probe>=0 || error!=EBUSY) {
            if (probe>=0) close(probe);
            std::fprintf(stderr,"P100A_Q35_SUPPORTED_SPAN exclusive_check_failed errno=%d\n",error);
            close(fd);return -1;
        }
        return fd;
    }
    bool Call(int fd,uint32_t op,void* data,size_t size) {
        if (ioctl(fd,op,data,size)==0) return true;
        std::fprintf(stderr,"P100A_Q35_SUPPORTED_SPAN ioctl=0x%08x errno=%d\n",op,errno);
        return false;
    }
    void Failure(const char* stage,unsigned bank) {
        std::fprintf(stderr,"P100A_Q35_SUPPORTED_SPAN failed_stage=%s bank=%u\n",stage,bank);
    }
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
