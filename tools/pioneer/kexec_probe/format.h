/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef PIONEER_KEXEC_PROBE_FORMAT_H
#define PIONEER_KEXEC_PROBE_FORMAT_H

typedef unsigned int ProbeU32;
typedef unsigned long long ProbeU64;

#define PROBE_FDT_HEADER_SIZE 40
#define PROBE_FDT_MAX_SIZE (2u * 1024u * 1024u)

struct ProbeFdtHeader {
	ProbeU32 totalSize;
	ProbeU32 version;
	ProbeU32 bootCpu;
};

/* Header/bounds checks only, not validation of the full FDT structure. */
const char* probe_check_fdt_header(const unsigned char* data,
	unsigned long available, struct ProbeFdtHeader* result);
int probe_pioneer_ram_contains(ProbeU64 address, ProbeU64 size);

#endif
