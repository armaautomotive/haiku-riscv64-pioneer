/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include "PioneerQ38RgdnRootDebug.h"
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
struct RgdnRootIO {
 int Open() { return open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC); }
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  if (!ioctl(fd,op,data,bytes)) return true;
  fprintf(stderr,"STOP:RGDN_ROOT op=%08x errno=%d;CLOSE_ONLY_NO_RETRY_RESET\n",op,errno); return false;
 }
 bool Close(int fd) { return close(fd)==0; }
};
int main(int argc,char** argv) {
 if (argc!=2) return 2;
 bool support=!std::strcmp(argv[1],"--support");
 unsigned trials=!std::strcmp(argv[1],"--pilot")?1:!std::strcmp(argv[1],"--synthetic")?16:0;
 if (!support && !trials) return 2;
 setvbuf(stdout,nullptr,_IOLBF,0); RgdnRootIO io;
 return PioneerQ38RgdnRootDebug(io,trials?trials:1,support)?0:1;
}
