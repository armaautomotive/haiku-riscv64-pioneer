#include <StartupTiming.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

static bigtime_t sNow;
static std::vector<std::string> sOutput;

bigtime_t system_time() { return sNow; }

extern "C" void syslog(int priority, const char* format, ...)
{
	assert(priority == LOG_INFO);
	char buffer[1024];
	va_list arguments;
	va_start(arguments, format);
	int length = vsnprintf(buffer, sizeof(buffer), format, arguments);
	va_end(arguments);
	assert(length > 0 && (size_t)length < sizeof(buffer));
	sOutput.push_back(buffer);
}

int main()
{
	sNow = 100;
	BPrivate::StartupTiming timing("test");
	sNow = 120;
	timing.Mark("first");
	sNow = 150;
	timing.Mark("second");
	assert(sOutput.empty());
	sNow = 900; // Flush time must not replace captured timestamps.
	timing.Flush();
	assert(sOutput.size() == 2);
	assert(sOutput[0].find("start_us=100 at_us=120 delta_us=20 dropped=0")
		!= std::string::npos);
	assert(sOutput[1].find("at_us=150 delta_us=30") != std::string::npos);
	sOutput.clear();
	BPrivate::StartupTiming full("capacity");
	for (int i = 0; i < 18; i++) {
		sNow++;
		full.Mark("stage");
	}
	assert(sOutput.empty());
	full.Flush();
	assert(sOutput.size() == 16);
	for (size_t i = 0; i < sOutput.size(); i++)
		assert(sOutput[i].find("dropped=2") != std::string::npos);
	sOutput.clear();
	BPrivate::StartupTiming empty("empty");
	empty.Flush();
	assert(sOutput.empty());
	puts("PASS: startup timing collection, deferred output, overflow, empty log");
}
