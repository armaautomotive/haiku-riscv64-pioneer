/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "Protocol.h"
#include <Entry.h>
#include <InputServerFilter.h>
#include <InterfaceDefs.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>

class ShortcutDispatcher : public BLooper {
public:
	ShortcutDispatcher() : BLooper("assistant shortcut") {}
	virtual void MessageReceived(BMessage* message)
	{
		if (message->what != kToggle) {
			BLooper::MessageReceived(message);
			return;
		}
		BMessenger app(kAssistantSignature);
		if (app.IsValid()) {
			app.SendMessage(kToggle);
			return;
		}
		entry_ref ref;
		if (get_ref_for_path("/boot/home/config/non-packaged/apps/HaikuAssistant", &ref) == B_OK)
			be_roster->Launch(&ref);
	}
};

class AssistantShortcut : public BInputServerFilter {
public:
	AssistantShortcut() : fDispatcher(new ShortcutDispatcher), fHeld(-1)
	{
		fDispatcher->Run();
		fTarget = BMessenger(fDispatcher);
	}
	virtual ~AssistantShortcut()
	{
		if (fDispatcher->Lock()) fDispatcher->Quit();
	}
	virtual status_t InitCheck() { return fTarget.IsValid() ? B_OK : B_ERROR; }
	virtual filter_result Filter(BMessage* message, BList*)
	{
		int32 key;
		if (message->FindInt32("key", &key) != B_OK) return B_DISPATCH_MESSAGE;
		if (message->what == B_KEY_UP && key == fHeld) {
			fHeld = -1;
			return B_SKIP_MESSAGE;
		}
		if (message->what != B_KEY_DOWN) return B_DISPATCH_MESSAGE;
		if (key == fHeld) return B_SKIP_MESSAGE;
		int32 raw, modifiers;
		if (message->FindInt32("raw_char", &raw) != B_OK
			|| message->FindInt32("modifiers", &modifiers) != B_OK)
			return B_DISPATCH_MESSAGE;
		int32 held = modifiers & (B_CONTROL_KEY | B_COMMAND_KEY | B_OPTION_KEY
			| B_SHIFT_KEY | B_MENU_KEY);
		if (raw != B_SPACE || held != B_CONTROL_KEY) return B_DISPATCH_MESSAGE;
		BMessage toggle(kToggle);
		if (fTarget.SendMessage(&toggle, (BHandler*)NULL, 0) != B_OK)
			return B_DISPATCH_MESSAGE;
		fHeld = key;
		return B_SKIP_MESSAGE;
	}
private:
	ShortcutDispatcher* fDispatcher;
	BMessenger fTarget;
	int32 fHeld;
};

extern "C" BInputServerFilter* instantiate_input_filter()
{
	return new AssistantShortcut;
}
