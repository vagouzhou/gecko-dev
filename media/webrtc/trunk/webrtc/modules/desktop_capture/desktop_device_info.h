/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DEVICE_INFO_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DEVICE_INFO_H_

#include <map>
#include "webrtc/modules/desktop_capture/desktop_capture_types.h"

namespace webrtc {

class DesktopDevice {
public:
	DesktopDevice();
	~DesktopDevice();

	void setDeviceName(const char *deviceNameUTF8);
	void setUniqueIdName(const char *deviceUniqueIdUTF8);

	const char *getDeviceName();
	const char *getUniqueIdName();

	DesktopDevice& operator= (DesktopDevice& other);

protected:
	char* deviceNameUTF8_;
	char* deviceUniqueIdUTF8_;
};

class DesktopDisplay : public DesktopDevice{
public:
	DesktopDisplay():screenId_(kInvalidScreenId){}
	~DesktopDisplay(){}

	void setScreenId(const ScreenId screenId){ screenId_=screenId; }
	ScreenId getScreenId(){ return screenId_; }

	DesktopDisplay& operator= (DesktopDisplay& other);

protected:
	ScreenId screenId_;
};
typedef std::map<intptr_t,DesktopDisplay*> DesktopDisplayList;

class DesktopWindow : public DesktopDevice{
public:
	DesktopWindow():windowId_(kNullWindowId){}
	~DesktopWindow(){}
	void setWindowId(const WindowId windowId){ windowId_ = windowId; };
	WindowId getWindowId(){ return windowId_; }

	DesktopWindow& operator= (DesktopWindow& other);
protected:
	WindowId windowId_;
};
typedef std::map<intptr_t,DesktopWindow*> DesktopWindowList;

class DesktopApplication : public DesktopDevice{
public:
	DesktopApplication();
	~DesktopApplication();

	void setProcessId(const ProcessId processId);
	void setProcessPathName(const char *appPathNameUTF8);

	ProcessId getProcessId();
	const char *getProcessPathName();

	DesktopApplication& operator= (DesktopApplication& other);

protected:
	ProcessId processId_;
	char* processPathNameUTF8_;
};

typedef std::map<intptr_t,DesktopApplication*> DesktopApplicationList;

class DesktopDeviceInfo {
public:
	virtual ~DesktopDeviceInfo() {};

	virtual int32_t Init() = 0;
	virtual int32_t Refresh() = 0;
	virtual int32_t getDisplayDeviceCount() = 0;
	virtual int32_t getDesktopDisplayDeviceInfo(int32_t nIndex,
		DesktopDisplay & desktopDisplayDevice) = 0;
	virtual int32_t getWindowCount() = 0;
	virtual int32_t getWindowInfo(int32_t nindex,
		DesktopWindow &windowDevice) = 0;
	virtual int32_t getApplicationCount() = 0;
	virtual int32_t getApplicationInfo(int32_t nIndex,
		DesktopApplication & desktopApplication) = 0;
};

class DesktopDeviceInfoImpl : public DesktopDeviceInfo {
public:
	DesktopDeviceInfoImpl();
	~DesktopDeviceInfoImpl();

	virtual int32_t Init();
	virtual int32_t Refresh();
	virtual int32_t getDisplayDeviceCount();
	virtual int32_t getDesktopDisplayDeviceInfo(int32_t nIndex,
		DesktopDisplay & desktopDisplayDevice);
	virtual int32_t getWindowCount();
	virtual int32_t getWindowInfo(int32_t nindex,
		DesktopWindow &windowDevice);
	virtual int32_t getApplicationCount();
	virtual int32_t getApplicationInfo(int32_t nIndex,
		DesktopApplication & desktopApplication);

	static DesktopDeviceInfo * Create();
protected:
	virtual int32_t RefreshWindowList();
	virtual int32_t RefreshScreenList();
	virtual int32_t RefreshApplicationList();

	void CleanUp();
	void CleanUpWindowList();
	void CleanUpScreenList();
	void CleanUpApplicationList();
protected:
	DesktopDisplayList desktop_display_list_;
	DesktopWindowList desktop_window_list_;
	DesktopApplicationList desktop_application_list_;
};
};

#endif
