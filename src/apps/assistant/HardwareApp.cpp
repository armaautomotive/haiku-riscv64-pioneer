/* Copyright 2026. Distributed under the terms of the MIT License. */
#include "HardwareWindow.h"
#include <Application.h>

class HardwareApp : public BApplication {
public:
	HardwareApp() : BApplication(kHardwareSignature) {}
	virtual void ReadyToRun()
	{
		HardwareWindow* window = new HardwareWindow(true);
		fWindow = BMessenger(window); window->Show();
	}
	virtual void ArgvReceived(int32, char**) { fWindow.SendMessage(kShowHardwareWindow); }
private:
	BMessenger fWindow;
};

int main()
{
	HardwareApp app; app.Run(); return 0;
}
