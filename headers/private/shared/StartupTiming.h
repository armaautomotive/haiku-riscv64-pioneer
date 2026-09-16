/* Distributed under the terms of the MIT License. */
#ifndef _PRIVATE_STARTUP_TIMING_H
#define _PRIVATE_STARTUP_TIMING_H

#include <OS.h>
#include <syslog.h>
#include <unistd.h>

namespace BPrivate {

// Stack-local, single-threaded startup measurements. Labels have static
// lifetime. Mark() never allocates or logs. Flush only after the measured work;
// these are callback milestones, not proof of service or display readiness.
class StartupTiming {
public:
	explicit StartupTiming(const char* component)
		:
		fComponent(component),
		fStart(system_time()),
		fCount(0),
		fDropped(0)
	{
	}

	void Mark(const char* stage)
	{
		if (fCount == kCapacity) {
			fDropped++;
			return;
		}
		fRecords[fCount].stage = stage;
		fRecords[fCount++].time = system_time();
	}

	void Flush() const
	{
		bigtime_t previous = fStart;
		for (uint32 i = 0; i < fCount; i++) {
			const Record& record = fRecords[i];
			syslog(LOG_INFO, "BOOT_TIMING v=1 component=%s team=%ld "
				"stage=%s start_us=%" B_PRIdBIGTIME " at_us=%" B_PRIdBIGTIME
				" delta_us=%" B_PRIdBIGTIME " dropped=%" B_PRIu32,
				fComponent, (long)getpid(), record.stage, fStart, record.time,
				record.time - previous, fDropped);
			previous = record.time;
		}
	}

private:
	enum { kCapacity = 16 };
	struct Record {
		const char* stage;
		bigtime_t time;
	};
	const char* fComponent;
	bigtime_t fStart;
	uint32 fCount;
	uint32 fDropped;
	Record fRecords[kCapacity];
};

} // namespace BPrivate

#endif
