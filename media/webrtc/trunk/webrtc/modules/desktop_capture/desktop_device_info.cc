/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webrtc/modules/desktop_capture/desktop_device_info.h"
#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace webrtc {

static inline void SetStringMember(char **member, const char *value) {
	if (!value) {
		return;
	}

	if (*member) {
		delete [] *member;
		*member = NULL;
	}

	size_t  nBufLen = strlen(value) + 1;
	char *buffer = new char[nBufLen];
	memcpy(buffer, value, nBufLen - 1);
	buffer[nBufLen - 1] = '\0';
	*member = buffer;
}

DesktopDevice::DesktopDevice() {
	deviceUniqueIdUTF8_ = NULL;
	deviceNameUTF8_ = NULL;
}

DesktopDevice::~DesktopDevice() {

	if (deviceUniqueIdUTF8_){
		delete [] deviceUniqueIdUTF8_;
	}

	if (deviceNameUTF8_){
		delete [] deviceNameUTF8_;
	}

	deviceUniqueIdUTF8_ = NULL;
	deviceNameUTF8_ = NULL;
}

void DesktopDevice::setDeviceName(const char *deviceNameUTF8) {
	SetStringMember(&deviceNameUTF8_, deviceNameUTF8);
}

void DesktopDevice::setUniqueIdName(const char *deviceUniqueIdUTF8) {
	SetStringMember(&deviceUniqueIdUTF8_, deviceUniqueIdUTF8);
}

const char *DesktopDevice::getDeviceName() {
	return deviceNameUTF8_;
}

const char *DesktopDevice::getUniqueIdName() {
	return deviceUniqueIdUTF8_;
}

DesktopDevice& DesktopDevice::operator= (DesktopDevice& other) {
	setUniqueIdName(other.getUniqueIdName());
	setDeviceName(other.getDeviceName());

	return *this;
}

DesktopDisplay& DesktopDisplay::operator= (DesktopDisplay& other){
	DesktopDevice::operator = (other);
	screenId_ = other.getScreenId();

	return *this;
}

DesktopWindow& DesktopWindow::operator= (DesktopWindow& other){
	DesktopDevice::operator = (other);
	windowId_ = other.getWindowId();

	return *this;
}

DesktopApplication::DesktopApplication() {
	processId_ = kNullProcessId;
	processPathNameUTF8_= NULL;
}

DesktopApplication::~DesktopApplication() {
	if(processPathNameUTF8_) 
		delete processPathNameUTF8_;
	processPathNameUTF8_ = NULL;
}

void DesktopApplication::setProcessId(const ProcessId processId) {
	processId_ = processId;
}

void DesktopApplication::setProcessPathName(const char *appPathNameUTF8) {
	SetStringMember(&processPathNameUTF8_, appPathNameUTF8);
}

ProcessId DesktopApplication::getProcessId() {
	return processId_;
}

const char *DesktopApplication::getProcessPathName() {
	return processPathNameUTF8_;
}

DesktopApplication& DesktopApplication::operator= (DesktopApplication& other) {
	DesktopDevice::operator = (other);
	processId_ = other.getProcessId();
	setProcessPathName(other.getProcessPathName());

	return *this;
}

DesktopDeviceInfoImpl::DesktopDeviceInfoImpl() {
}

DesktopDeviceInfoImpl::~DesktopDeviceInfoImpl() {
	CleanUp();
}

int32_t DesktopDeviceInfoImpl::getDisplayDeviceCount() {
	RefreshScreenList();
	return desktop_display_list_.size();
}

int32_t DesktopDeviceInfoImpl::Init() {
	//Refresh();
	return 0;
}

int32_t DesktopDeviceInfoImpl::Refresh() {
	RefreshWindowList();
	RefreshScreenList();
	RefreshApplicationList();
	return 0;
}

int32_t DesktopDeviceInfoImpl::getDesktopDisplayDeviceInfo(int32_t nIndex,
				DesktopDisplay & desktopDisplayDevice) {

	if(nIndex < 0 || nIndex >= desktop_display_list_.size()) {
		return -1;
	}

	std::map<intptr_t,DesktopDisplay*>::iterator iter = desktop_display_list_.begin();
	std::advance (iter, nIndex);
	DesktopDisplay * pDesktopDisplayDevice = iter->second;
	if(pDesktopDisplayDevice) {
		desktopDisplayDevice = (*pDesktopDisplayDevice);
	}

	return 0;
}

int32_t DesktopDeviceInfoImpl::getWindowCount() {
	RefreshWindowList();
	return desktop_window_list_.size();
}

int32_t DesktopDeviceInfoImpl::getWindowInfo(int32_t nIndex,
	DesktopWindow &windowDevice) {
	if (nIndex < 0 || nIndex >= desktop_window_list_.size()) {
		return -1;
	}

	std::map<intptr_t, DesktopWindow *>::iterator itr = desktop_window_list_.begin();
	std::advance(itr, nIndex);
	DesktopWindow * pWindow = itr->second;
	if (!pWindow) {
		return -1;
	}

	windowDevice = (*pWindow);
	return 0;
}

int32_t DesktopDeviceInfoImpl::getApplicationCount() {
	RefreshApplicationList();
	return desktop_application_list_.size();
}

int32_t DesktopDeviceInfoImpl::getApplicationInfo(int32_t nIndex,
	DesktopApplication & desktopApplication) {
	if(nIndex < 0 || nIndex >= desktop_application_list_.size()) {
		return -1;
	}

	std::map<intptr_t,DesktopApplication*>::iterator iter = desktop_application_list_.begin();
	std::advance (iter, nIndex);
	DesktopApplication * pDesktopApplication = iter->second;
	if (pDesktopApplication) {
		desktopApplication = (*pDesktopApplication);
	}

	return 0;
}
void DesktopDeviceInfoImpl::CleanUp(){
	CleanUpWindowList();
	CleanUpScreenList();
	CleanUpApplicationList();	
}

void DesktopDeviceInfoImpl::CleanUpWindowList(){
	std::map<intptr_t, DesktopWindow*>::iterator iterWindow;
	for (iterWindow = desktop_window_list_.begin(); iterWindow != desktop_window_list_.end(); iterWindow++) {
		DesktopWindow * pWindow = iterWindow->second;
		delete pWindow;
		iterWindow->second = NULL;
	}
	desktop_window_list_.clear();
}

void DesktopDeviceInfoImpl::CleanUpScreenList(){
	std::map<intptr_t,DesktopDisplay*>::iterator iterDevice;
	for (iterDevice=desktop_display_list_.begin(); iterDevice!=desktop_display_list_.end(); iterDevice++){
		DesktopDisplay * pDesktopDisplayDevice = iterDevice->second;
		delete pDesktopDisplayDevice;
		iterDevice->second = NULL;
	}
	desktop_display_list_.clear();
}

void DesktopDeviceInfoImpl::CleanUpApplicationList(){
	std::map<intptr_t,DesktopApplication*>::iterator iterApp;
	for (iterApp=desktop_application_list_.begin(); iterApp!=desktop_application_list_.end(); iterApp++){
		DesktopApplication * pDesktopApplication = iterApp->second;
		delete pDesktopApplication;
		iterApp->second = NULL;
	}
	desktop_application_list_.clear();
}
/*vagouzhou@gmail.com >> we need refactoring it 
should not get windows list from capturer.
should refactoring it by moving logic of GetWindowList from capturer to desktop_device_info_xplatform.
then capturer use the device_info, not device_info use capturer. 
RefreshWindowList & RefreshScreenList will be as RefreshApplicationList.implement it in the desktop_device_info_xplatform
*/
int32_t DesktopDeviceInfoImpl::RefreshWindowList() {
	CleanUpWindowList();

	//List all Windows
	scoped_ptr<WindowCapturer> pWinCap(WindowCapturer::Create());
	WindowCapturer::WindowList list;
	if (pWinCap && pWinCap->GetWindowList(&list)) {
		WindowCapturer::WindowList::iterator itr;
		for (itr = list.begin(); itr != list.end(); itr++) {
			DesktopWindow *pWinDevice = new DesktopWindow;
			if (!pWinDevice) {
				continue;
			}
			WindowId windowId = itr->id;
			pWinDevice->setWindowId(windowId);
			pWinDevice->setDeviceName(itr->title.c_str());

			std::ostringstream s;
			s<<windowId;
			pWinDevice->setUniqueIdName(s.str().c_str());
			desktop_window_list_[pWinDevice->getWindowId()] = pWinDevice;
		}
	}

	return 0;
}

int32_t DesktopDeviceInfoImpl::RefreshScreenList(){
	CleanUpScreenList();

	//List all Windows
#if !defined(MULTI_MONITOR_SCREENSHARE)
	DesktopDisplay *pDesktopDeviceInfo = new DesktopDisplay;
	if (pDesktopDeviceInfo) {
		ScreenId screenId = webrtc::kFullDesktopScreenId;
		pDesktopDeviceInfo->setScreenId(screenId);
		pDesktopDeviceInfo->setDeviceName("Entire Screen");
		std::ostringstream s;
		s<<screenId;
		pDesktopDeviceInfo->setUniqueIdName(s.str().c_str());

		desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
	}
#endif
	return 0;
}
int32_t DesktopDeviceInfoImpl::RefreshApplicationList(){
	//implement in desktop_device_info_xplatform
	return 0;
}

}
