/* Copyright 2026. Distributed under the terms of the MIT License. */
#ifndef HAIKU_ASSISTANT_MODEL_MANAGER_H
#define HAIKU_ASSISTANT_MODEL_MANAGER_H

#include <Messenger.h>
#include <Window.h>
#include <atomic>

class BButton;
class BListView;
class BMessageRunner;
class BStatusBar;
class BStringView;
class BTextView;
static const uint32 kShowModelManager = 'mshw';

class ModelManager : public BWindow {
public:
	ModelManager();
	virtual ~ModelManager();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);
private:
	void Refresh();
	void Details();
	void Install();
	void Select();
	void ModelMemory(uint32 action);
	void PollService();
	static int32 Download(void* data);
	BListView* fList;
	BTextView* fDetails;
	BStringView* fStatus;
	BStatusBar* fProgress;
	BStatusBar* fLoadProgress;
	BStringView* fMemoryModel;
	BStringView* fMemoryStatus;
	BButton* fLoad;
	BButton* fUnload;
	BButton* fInstall;
	BButton* fSelect;
	BButton* fCancel;
	BMessageRunner* fPoll;
	thread_id fWorker;
	int32 fDownloadIndex;
	bool fDownloading;
	bool fSelecting;
	bool fServiceBusy;
	std::atomic<bool> fCancelled;
};

#endif
