#ifndef HAIKU_ASSISTANT_TOOL_SESSION_H
#define HAIKU_ASSISTANT_TOOL_SESSION_H
#include <Looper.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <string>

class ToolSession : public BLooper {
public:
	ToolSession();
	virtual ~ToolSession();
	virtual void MessageReceived(BMessage* message);
private:
	void SendModel();
	void Stop();
	void Publish(uint32 what, const std::string& text, const char* status, bool success = false);
	void Fail(const char* status);
	BMessenger fClient;
	BMessenger fModel;
	BMessenger fTools;
	int64 fOuterID, fInnerID;
	bool fBusy, fWaitingTool;
	int fToolCount;
	std::string fPrompt, fVisible, fCall;
	BMessageRunner* fWatch;
	bigtime_t fDeadline;
};
#endif
