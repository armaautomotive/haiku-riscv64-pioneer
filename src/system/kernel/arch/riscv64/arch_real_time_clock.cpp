/*
 * Copyright 2021 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <arch/real_time_clock.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <debug.h>

#include <real_time_clock.h>
#include <real_time_data.h>

#include <Htif.h>


extern uint32 gPlatform;


status_t
arch_rtc_init(kernel_args *args, struct real_time_data *data)
{
	data->arch_data.system_time_conversion_factor
		= (1LL << 32) * 1000000LL / args->arch_args.timerFrequency;
	debug_early_boot_message("riscv: rtc conversion factor ready\n");
	return B_OK;
}


uint64
arch_rtc_get_hw_time(void)
{
	if (gPlatform == kPlatformSbi) {
		debug_early_boot_message("riscv: SBI RTC unavailable, using epoch\n");
		return 0;
	}

	return (HtifCmd(2, 0, 0) / 1000000);
}


void
arch_rtc_set_hw_time(uint64 seconds)
{
}


void
arch_rtc_set_system_time_offset(struct real_time_data *data, bigtime_t offset)
{
	atomic_set64(&data->arch_data.system_time_offset, offset);
}


bigtime_t
arch_rtc_get_system_time_offset(struct real_time_data *data)
{
	return atomic_get64(&data->arch_data.system_time_offset);
}
