/* SPDX-License-Identifier: MIT */
/*
 * Standalone Linux RV64 init for the Pioneer firmware-only PCIe test.
 * No shell, block devices, network, kexec, PCI writes, or boot fallback.
 * The firmware kernel still handles normal PCI enumeration before this runs.
 */
typedef unsigned long usize;

static long
Call(long nr, long x0, long x1, long x2, long x3, long x4)
{
	register long a0 __asm__("a0") = x0;
	register long a1 __asm__("a1") = x1;
	register long a2 __asm__("a2") = x2;
	register long a3 __asm__("a3") = x3;
	register long a4 __asm__("a4") = x4;
	register long a7 __asm__("a7") = nr;
	__asm__ volatile("ecall" : "+r"(a0)
		: "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7) : "memory");
	return a0;
}

static usize
Length(const char* s)
{
	usize n = 0;
	while (s[n] != '\0')
		n++;
	return n;
}

static void
Print(const char* s)
{
	usize len = Length(s);
	while (len != 0) {
		long n = Call(64, 1, (long)s, (long)len, 0, 0); /* write stdout */
		if (n <= 0)
			return;
		s += n;
		len -= (usize)n;
	}
}

static int
Equal(const char* a, const char* b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static long
Open(const char* path)
{
	return Call(56, -100, (long)path, 0, 0, 0); /* openat, O_RDONLY */
}

static int
ReadField(const char* bdf, const char* field, char* out)
{
	const char* prefix = "/sys/bus/pci/devices/";
	char path[96];
	usize p = 0;
	while (*prefix != '\0')
		path[p++] = *prefix++;
	while (*bdf != '\0')
		path[p++] = *bdf++;
	path[p++] = '/';
	while (*field != '\0')
		path[p++] = *field++;
	path[p] = '\0';
	long fd = Open(path);
	if (fd < 0)
		return 0;
	long n = Call(63, fd, (long)out, 39, 0, 0); /* read */
	Call(57, fd, 0, 0, 0, 0); /* close */
	if (n <= 0)
		return 0;
	if (out[n - 1] == '\n')
		n--;
	out[n] = '\0';
	return 1;
}

static int
ValidBdf(const char* name)
{
	if (Length(name) != 12)
		return 0;
	for (unsigned i = 0; i < 12; i++) {
		char c = name[i];
		if (i == 4 || i == 7) {
			if (c != ':')
				return 0;
		} else if (i == 10) {
			if (c != '.')
				return 0;
		} else if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
			return 0;
	}
	return name[11] >= '0' && name[11] <= '7';
}

static void
PrintField(const char* name, const char* field)
{
	char value[40];
	if (ReadField(name, field, value)) {
		Print(" ");
		Print(field);
		Print("=");
		Print(value);
	}
}

static int
Report(const char* name)
{
	char vendor[40], device[40];
	if (!ValidBdf(name))
		return 0;
	if (!ReadField(name, "vendor", vendor) || !ReadField(name, "device", device))
		return -1;
	int blackhole = Equal(vendor, "0x1e52") && Equal(device, "0xb140");
	Print("PCIE_DEVICE ");
	Print(name);
	Print(" ");
	Print(vendor);
	Print(":");
	Print(device);
	if (blackhole) {
		PrintField(name, "subsystem_vendor");
		PrintField(name, "subsystem_device");
	}
	if (blackhole || Equal(vendor, "0x1e30")) {
		PrintField(name, "current_link_speed");
		PrintField(name, "current_link_width");
		PrintField(name, "max_link_speed");
		PrintField(name, "max_link_width");
	}
	Print("\n");
	return blackhole;
}

/* The entry never returns; no libc or runtime initialization is required. */
void
_start(void)
{
	if (Call(172, 0, 0, 0, 0, 0) != 1) /* getpid */
		Call(93, 2, 0, 0, 0, 0); /* exit: must be the init process */
	Print("\nPIONEER_PCIE_SD_PROBE_BEGIN v1\n");
	long mountResult = Call(40, (long)"sysfs", (long)"/sys", (long)"sysfs",
		15, 0); /* mount sysfs read-only, nosuid, nodev, noexec */
	long fd = mountResult == 0 ? Open("/sys/bus/pci/devices") : -1;
	int found = 0;
	int valid = fd >= 0;
	unsigned char buf[2048];
	while (valid) {
		long n = Call(61, fd, (long)buf, sizeof(buf), 0, 0); /* getdents64 */
		if (n <= 0) {
			valid = n == 0;
			break;
		}
		for (long at = 0; at < n;) {
			if (n - at < 20) {
				valid = 0;
				break;
			}
			unsigned len = buf[at + 16] | (unsigned)buf[at + 17] << 8;
			if (len < 20 || len > n - at) {
				valid = 0;
				break;
			}
			unsigned end = 19;
			while (end < len && buf[at + end] != 0)
				end++;
			if (end == len) {
				valid = 0;
				break;
			}
			int match = Report((const char*)&buf[at + 19]);
			if (match < 0) {
				valid = 0;
				break;
			}
			found += match;
			at += len;
		}
	}
	if (fd >= 0)
		Call(57, fd, 0, 0, 0, 0);
	if (!valid)
		Print("P100A_INVENTORY_UNAVAILABLE\n");
	else if (found == 0)
		Print("P100A_NOT_ENUMERATED\n");
	else if (found == 1)
		Print("P100A_ENUMERATED\n");
	else
		Print("P100A_MULTIPLE_ENDPOINTS\n");
	Print("PIONEER_PCIE_SD_PROBE_PARKED; NO_INSTALLED_OS_STARTED\n");
	for (;;)
		Call(73, 0, 0, 0, 0, 0); /* ppoll: wait indefinitely */
}
