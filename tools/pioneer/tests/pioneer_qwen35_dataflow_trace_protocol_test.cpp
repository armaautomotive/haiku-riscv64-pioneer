/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQwen35DataflowProtocol.h"
#include <cassert>
int main() {
	tt_qwen35_dataflow_request request{};
	request.version=TT_QWEN35_DATAFLOW_ABI;
	request.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_RESULT;
	assert(PioneerQwen35DataflowRequestValid(request));
	request.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_TRACE;
	for (unsigned slot=0;slot<TT_QWEN35_DATAFLOW_GRAPH_TRACE_SLOTS;slot++) {
		request.next_sequence=slot;assert(PioneerQwen35DataflowRequestValid(request));
	}
	request.next_sequence=TT_QWEN35_DATAFLOW_GRAPH_TRACE_SLOTS;
	assert(!PioneerQwen35DataflowRequestValid(request));
	request.next_sequence=0;request.reserved[0]=1;
	assert(!PioneerQwen35DataflowRequestValid(request));
	request.reserved[0]=0;request.action=TT_QWEN35_DATAFLOW_ACTION_QUERY_GRAPH_TRACE+1;
	assert(!PioneerQwen35DataflowRequestValid(request));
}
