/* SPDX-License-Identifier: MIT */
/* Offline resident-graph witness. Never installed by the stable driver. */
.option norvc
.option norelax
.section .boot,"ax"
.global _start
_start:
    j qwen35_dataflow_entry
.section .text.start,"ax"
.global qwen35_dataflow_entry, qwen35_resident_fault_park
qwen35_dataflow_entry:
    li sp,0x70000
    call qwen35_resident_graph_service_main
qwen35_resident_fault_park:
    j qwen35_resident_fault_park
    j qwen35_resident_fault_park
