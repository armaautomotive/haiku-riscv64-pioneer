#!/usr/bin/env python3
"""Check that the simulator exposes seven distinct Pioneer DRAM routes."""
import ctypes
import struct
import sys


def main():
    if len(sys.argv) != 2:
        raise SystemExit('usage: bank_routes_test.py /absolute/libttsim.so')
    lib = ctypes.CDLL(sys.argv[1])
    lib.libttsim_pci_config_rd32.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.libttsim_pci_config_rd32.restype = ctypes.c_uint32
    lib.libttsim_pci_mem_wr_bytes.argtypes = [ctypes.c_uint64, ctypes.c_void_p, ctypes.c_uint32]
    lib.libttsim_pci_mem_rd_bytes.argtypes = [ctypes.c_uint64, ctypes.c_void_p, ctypes.c_uint32]
    lib.libttsim_init()
    try:
        if lib.libttsim_pci_config_rd32(0, 0) != 0xb1401e52:
            raise RuntimeError('simulator is not Blackhole')
        bar = ((lib.libttsim_pci_config_rd32(0, 0x14) << 32)
               | (lib.libttsim_pci_config_rd32(0, 0x10) & ~15))

        def write(address, data):
            buf = ctypes.create_string_buffer(data, len(data))
            lib.libttsim_pci_mem_wr_bytes(address, buf, len(data))

        def read(address, size):
            buf = ctypes.create_string_buffer(size)
            lib.libttsim_pci_mem_rd_bytes(address, buf, size)
            return buf.raw

        def select(route, address):
            page = address >> 21
            for word, value in enumerate((page & 0xffffffff, (page >> 32) | (route << 11), 0)):
                write(bar + 0x1fc00000 + 2 * 12 + word * 4, struct.pack('<I', value))

        routes = (786, 978, 1170, 785, 977, 1169, 1361)
        addresses = (0x08000000, 0x081fffc0, 0x08200000)
        for bank, route in enumerate(routes):
            for index, address in enumerate(addresses):
                select(route, address)
                write(bar + 0x400000 + (address & 0x1fffff), bytes([bank * 3 + index + 1]) * 64)
        for bank, route in enumerate(routes):
            for index, address in enumerate(addresses):
                select(route, address)
                if read(bar + 0x400000 + (address & 0x1fffff), 64) != bytes([bank * 3 + index + 1]) * 64:
                    raise RuntimeError(f'simulator DRAM mismatch: bank {bank}, sample {index}')
        print('PASS seven distinct simulated DRAM routes across a TLB boundary; NO HARDWARE')
    finally:
        lib.libttsim_exit()


if __name__ == '__main__':
    main()
