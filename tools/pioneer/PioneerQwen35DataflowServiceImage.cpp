/* SPDX-License-Identifier: MIT */
#include "PioneerQwen35DataflowBlackholeWorker.h"

extern "C" [[noreturn]] void qwen35_dataflow_service_main()
{
	PioneerQwen35::PioneerQwen35DataflowBlackholeWorkerService();
}
