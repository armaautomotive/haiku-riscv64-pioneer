/* SPDX-License-Identifier: MIT */
// ACTUAL client, fake clock and finite fake worker. No real waiting/model/card.
#include <time.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
static long clockOffset=0;static bool clockFailed=false;
static int BudgetClock(clockid_t clock,timespec* value) {
	if(clockFailed)return -1;
	const int result=clock_gettime(clock,value);if(!result)value->tv_sec+=clockOffset;return result;
}
#define clock_gettime BudgetClock
#include "../../../src/apps/assistant/P100AQwen27.cpp"
#undef clock_gettime
static std::string Events(const char* path) {
	int fd=open(path,O_RDONLY);assert(fd>=0);char data[128];ssize_t size=read(fd,data,sizeof(data));
	assert(size>=0 && size<ssize_t(sizeof(data)));assert(!close(fd));return std::string(data,size);
}
int main() {
	struct Case{const char* mode;std::vector<long> advances;bool accepted;};
	const Case cases[]={
		{"clock-normal-ok",{1790},true},{"clock-normal-expired",{2000},false},
		{"clock-cache-long",{4000},true},{"clock-cache-edge",{7190},true},
		{"clock-cache-cap",{7300},false},{"clock-cache-repeat",{4000,7000,7300},false},
		{"clock-cache-value",{4000},false},{"clock-cache-text",{4000},false},
		{"clock-late-banner",{1900},false},{"clock-error",{-1},false},{"clock-start-error",{},false}};
	for(const auto& test:cases) {
		char log[]="/tmp/assistant27-load-budget-XXXXXX";int fd=mkstemp(log);assert(fd>=0);assert(!close(fd));
		assert(!setenv("ASSISTANT27_TEST_LOG",log,1) && !setenv("ASSISTANT27_TEST_MODE",test.mode,1));
		clockOffset=0;clockFailed=!strcmp(test.mode,"clock-start-error");unsigned progressCalls=0;
		{
			AssistantP100AQwen27 client;std::string error;
			const bool accepted=client.Load("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",4,error,[&](float,const char*) {
				assert(progressCalls<test.advances.size());long advance=test.advances[progressCalls++];
				if(advance<0)clockFailed=true;else clockOffset=advance;
			});
			assert(progressCalls==test.advances.size() && accepted==test.accepted && client.Faulted()!=accepted);
			clockFailed=false;
			if(accepted)assert(client.Unload(error) && !client.Faulted());
			else {
				assert(error=="Qwen27B load/channel failed; no retry or CPU fallback.");
				assert(!client.Unload(error));
			}
		}
		// Only this test's single fake child exists. Reap bookkeeping ONLY,
		// never a signal/kill/retry. Successful Unload already reaped it.
		if(!test.accepted){int status=0;pid_t child=waitpid(-1,&status,0);if(child>0)assert(WIFEXITED(status) && WEXITSTATUS(status)==70);else assert(errno==ECHILD);}
		assert(Events(log)==(test.accepted?"load\nunload\n":"load\nclose-only\n"));assert(!unlink(log));
	}
	puts("PASS actual Assistant load budgets:11 fake-clock cases;ordinary1800s unchanged,exact cache opt-in fixed7200s total,queued Ready cannot bypass cap,repeat/wrong/late banners do not refresh,initial/later clock failure closes only;NO real wait/model/device IO");
}
