/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_ROLE_TEST_KERNEL_EXPORT_H
#define PIONEER_ROLE_TEST_KERNEL_EXPORT_H
#include <stdint.h>
#include <stddef.h>
typedef int32_t area_id;
typedef int32_t status_t;
typedef int32_t thread_id;
typedef int32_t team_id;
typedef uint64_t phys_addr_t;
typedef uint64_t uint64;
struct thread_info { team_id team; };
enum { B_OK=0, B_ANY_KERNEL_ADDRESS=1, B_KERNEL_READ_AREA=2,
    B_KERNEL_WRITE_AREA=4, B_UNCACHED_MEMORY=8 };
area_id map_physical_memory(const char*,phys_addr_t,size_t,uint32_t,uint32_t,void**);
status_t delete_area(area_id);
thread_id find_thread(const char*);
status_t get_thread_info(thread_id,thread_info*);
status_t release_sem(int32_t);
#endif
