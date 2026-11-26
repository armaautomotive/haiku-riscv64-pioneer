/* SPDX-License-Identifier: MIT */
#include "PioneerPciPinProbe.h"
#include <Errors.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main() {
	int fd = open("/dev/misc/pioneer_pci_pin_probe", O_RDONLY);
	if (fd < 0) { perror("open PCI pin probe"); return 1; }
	pioneer_pci_pin_probe_result r = {};
	int status = ioctl(fd, PIONEER_PCI_PIN_PROBE, &r, sizeof(r));
	int closed = close(fd);
	if (status != 0 || closed != 0) { perror("PCI pin probe ioctl/close"); return 1; }
	printf("PCI_PIN_PROBE version=%lu manager=%ld pin=%ld card=%ld port=%ld callbacks=%lu drivers=%lu\n",
		(unsigned long)r.version, (long)r.managerStatus, (long)r.pinStatus,
		(long)r.cardDriverStatus, (long)r.portDriverStatus,
		(unsigned long)r.callbacks, (unsigned long)r.matchingDrivers);
	return r.version == 1 && r.managerStatus == B_OK && r.pinStatus == B_OK
		&& r.cardDriverStatus == B_OK && r.portDriverStatus == B_OK
		&& r.callbacks == 1 && r.matchingDrivers == 1 ? 0 : 1;
}
