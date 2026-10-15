/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_HARDWARE_WINDOW_H
#define HAIKU_ASSISTANT_HARDWARE_WINDOW_H

#include "Hardware.h"
#include <String.h>
#include <Window.h>

class BButton;
class BListView;
class BMessageRunner;
class BStringView;
class BTextControl;
class BTextView;
static const uint32 kShowHardwareWindow = 'hshw';
static const char* const kHardwareSignature = "application/x-vnd.Haiku-AssistantHardware";

class HardwareWindow : public BWindow {
public:
	explicit HardwareWindow(bool quitAppOnClose = false);
	virtual ~HardwareWindow();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);
private:
	void Poll();
	void Details();
	void Apply();
	BListView* fList;
	BTextView* fDetails;
	BStringView* fActive;
	BStringView* fStatus;
	BTextControl* fThreads;
	BButton* fApply;
	BMessageRunner* fPoll;
	bool fQuitAppOnClose, fThreadsLoaded, fServiceAvailable, fBusy;
	bool fAvailable[kAssistantHardwareCount];
	BString fDescriptions[kAssistantHardwareCount];
};

#endif
