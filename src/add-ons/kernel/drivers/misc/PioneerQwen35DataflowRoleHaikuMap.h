/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_ROLE_HAIKU_MAP_H
#define PIONEER_QWEN35_DATAFLOW_ROLE_HAIKU_MAP_H
#include <KernelExport.h>
#include <ByteOrder.h>
#include <stdint.h>
// Concrete read-only Haiku map primitives for the unbound role-view witness.
// The real session must provide pinned-device and selected-route guards. No
// ioctl, driver operation, mapping call or hardware access invokes this yet.
struct PioneerQwen35RoleHaikuMap {
    bool MapReadOnly(uint64_t physical,unsigned bytes,int32_t& area,
        const volatile uint32_t*& output) {
        if (uint64_t(phys_addr_t(physical))!=physical || !bytes) return false;
        void* view=NULL;
        const area_id result=map_physical_memory("P100A Qwen35 role read-only witness",
            phys_addr_t(physical),bytes,B_ANY_KERNEL_ADDRESS,
            B_KERNEL_READ_AREA|B_UNCACHED_MEMORY,&view);
        area=int32_t(result);
        output=static_cast<const volatile uint32_t*>(view);
        return result>=B_OK && view;
    }
    bool Unmap(int32_t area) { return area>=B_OK && delete_area(area_id(area))==B_OK; }
#ifdef PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY
#if !defined(PIONEER_QWEN35_RECOVERY_COMPILE_ONLY) || !defined(PIONEER_QWEN35_DEVICE_DISPATCH)
#error Writable role mappings are a non-loadable recovery witness only
#endif
    bool MapReadWrite(uint64_t physical,unsigned bytes,int32_t& area,
        volatile uint32_t*& output) {
        if (uint64_t(phys_addr_t(physical))!=physical || !bytes) return false;
        void* view=NULL;
        const area_id result=map_physical_memory("P100A Qwen35 role write witness",
            phys_addr_t(physical),bytes,B_ANY_KERNEL_ADDRESS,
            B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_UNCACHED_MEMORY,&view);
        area=int32_t(result); output=static_cast<volatile uint32_t*>(view);
        return result>=B_OK && view;
    }
    void WriteWord(volatile uint32_t* address,uint32_t value) {
#if defined(__riscv)
        asm volatile("fence iorw,iorw" ::: "memory");
#else
        __sync_synchronize();
#endif
        *address=B_HOST_TO_LENDIAN_INT32(value);
#if defined(__riscv)
        asm volatile("fence iorw,iorw" ::: "memory");
#else
        __sync_synchronize();
#endif
    }
#endif
    uint32_t ReadWord(const volatile uint32_t* address) {
#if defined(__riscv)
        asm volatile("fence iorw,iorw" ::: "memory");
#else
        __sync_synchronize();
#endif
        const uint32_t value=*address;
#if defined(__riscv)
        asm volatile("fence iorw,iorw" ::: "memory");
#else
        __sync_synchronize();
#endif
        return B_LENDIAN_TO_HOST_INT32(value);
    }
};
#endif
