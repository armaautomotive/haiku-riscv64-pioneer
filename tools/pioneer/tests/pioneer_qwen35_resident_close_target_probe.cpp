/* SPDX-License-Identifier: MIT */
// Haiku SDK type/stack witness; the device and physical scope are inert.
#include "pioneer_qwen35_dataflow_shared_service_kernel_probe.cpp"
struct ResidentCloseProbeScope {
    volatile bool okay=false;
    bool ResidentCloseAllowed(const void*,const DataflowServiceOwner&,SharedServiceKernelProbeIO&) { return okay; }
};
DataflowServiceResult PioneerResidentCloseKernelProbe(DataflowServiceOwner& owner,
    SharedServiceKernelProbeIO& io,const void* session,ResidentCloseProbeScope& scope) {
    return owner.CloseResident(io,session,scope);
}
