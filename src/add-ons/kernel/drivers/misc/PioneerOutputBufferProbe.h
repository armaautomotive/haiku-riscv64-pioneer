/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_OUTPUT_BUFFER_PROBE_H
#define PIONEER_OUTPUT_BUFFER_PROBE_H
#include "PioneerOutputBuffer.h"
// CPU-only allocator qualification. This interface contains NO PCI/MMIO,
// NoC, DMA, core-release or caller-address operation. No physical address is
// returned in its public result, and no area persists after a successful test.
enum { TT_OUTPUT_PROBE_REFUSED=0,TT_OUTPUT_PROBE_PASSED=1,
	TT_OUTPUT_PROBE_INPUT_FAILED=2,TT_OUTPUT_PROBE_OUTPUT_FAILED=3,TT_OUTPUT_PROBE_CLEANUP_FAILED=4 };
struct tt_output_buffer_probe_result {
	uint32_t version,state,input_state,output_state,input_allocated,output_allocated;
	uint32_t input_bytes,output_bytes,input_address_bits,output_address_bits;
	uint32_t input_patterns,output_patterns,input_mismatches,output_mismatches;
	uint32_t maps_stable,input_released,output_released,output_quarantined;
};
static_assert(sizeof(tt_output_buffer_probe_result)==72,"output-buffer probe returns fixed metadata, no addresses or pointers");
template<class IO>
bool PioneerOutputBufferProbe(IO& io,tt_output_buffer_probe_result& r)
{
	r={}; r.version=1; r.state=TT_OUTPUT_PROBE_INPUT_FAILED;
	tt_host_page_result input={}; PioneerOutputBufferLease output;
	bool ready=PioneerPrepareHostPage(io,input);
	if (ready) { r.state=TT_OUTPUT_PROBE_OUTPUT_FAILED; ready=PioneerPrepareOutputBuffer(io,input,output); }
	r.input_state=input.state; r.output_state=output.state;
	r.input_allocated=input.allocated; r.output_allocated=output.allocated;
	r.input_bytes=input.bytes; r.output_bytes=output.allocated?TT_OUTPUT_BUFFER_BYTES:0;
	r.input_address_bits=input.address_bits;
	for (uint64_t rest=output.physical;rest;rest>>=1) r.output_address_bits++;
	r.input_patterns=input.patterns; r.output_patterns=output.patterns;
	r.input_mismatches=input.mismatches; r.output_mismatches=output.mismatches;
	r.maps_stable=ready;
	// Known safe even after a preparation refusal: this probe has NEVER
	// issued any DMA or handed either private physical address to a device.
	bool outputReleased=!output.allocated || PioneerReleaseOutputBuffer(io,output,true);
	bool inputReleased=!input.allocated || io.ReleaseHostPage()==0;
	r.output_released=outputReleased; r.input_released=inputReleased;
	r.output_quarantined=output.quarantined;
	if (!outputReleased || !inputReleased) r.state=TT_OUTPUT_PROBE_CLEANUP_FAILED;
	else if (ready) r.state=TT_OUTPUT_PROBE_PASSED;
	return ready && outputReleased && inputReleased;
}
#endif
