/* SPDX-License-Identifier: MIT */
// Fixed reserved-state zero/readback qualifier ONLY,not a model runner.
#include <Drivers.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "PioneerQ38GdnZeroDebug.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
struct GdnZeroIO {
 int Open() { return open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); }
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  if (!ioctl(fd,op,data,bytes)) return true;
  fprintf(stderr,"STOP:GDN_ZERO op=%08x errno=%d;CLOSE_ONLY_NO_RETRY\n",op,errno); return false;
 }
 bool Close(int fd) { return close(fd)==0; }
};
int main(int argc,char** argv) {
 if (argc!=2) return 2;
 unsigned mode=!strcmp(argv[1],"--pilot")?1:!strcmp(argv[1],"--initialize")?2:0;
 bool support=!strcmp(argv[1],"--support"); if (!mode && !support) return 2;
 setvbuf(stdout,nullptr,_IOLBF,0); GdnZeroIO io;
 return PioneerQ38GdnZeroDebug(io,mode?mode:1,support)?0:1;
}
