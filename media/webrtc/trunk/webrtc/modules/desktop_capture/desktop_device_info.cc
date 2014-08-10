* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc{
    
    //================================================
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
    DesktopDisplayDevice::DesktopDisplayDevice(){
	screenId_ = kInvalidScreenId;
	deviceUniqueIdUTF8_ = NULL;
	deviceNameUTF8_ = NULL;
    }
    DesktopDisplayDevice::~DesktopDisplayDevice(){
	screenId_ = kInvalidScreenId;
        
	if (deviceUniqueIdUTF8_){
		delete [] deviceUniqueIdUTF8_;
	}
	if (deviceNameUTF8_){
		delete [] deviceNameUTF8_;
	}
        
	deviceUniqueIdUTF8_ = NULL;
	deviceNameUTF8_ = NULL;
    }
    void DesktopDisplayDevice::setScreenId(const ScreenId screenId)
    {
	screenId_ = screenId;
    }
    void DesktopDisplayDevice::setDeivceName(const char* deviceNameUTF8)
    {
        if(deviceNameUTF8==NULL) return;
        
        if(deviceNameUTF8_){
	SetStringMember(&deviceNameUTF8_, deviceNameUTF8);
            deviceNameUTF8_ = NULL;
        }
        int nBufLen = strlen(deviceNameUTF8)+1;
        deviceNameUTF8_ = new char[nBufLen];
        memset(deviceNameUTF8_,0,nBufLen);
        
        memcpy(deviceNameUTF8_,deviceNameUTF8,strlen(deviceNameUTF8));
	SetStringMember(&deviceUniqueIdUTF8_, deviceUniqueIdUTF8);
    }
    void DesktopDisplayDevice::setUniqueIdName(const char* deviceUniqueIdUTF8)
    {
        if(deviceUniqueIdUTF8==NULL) return;
        
        if(deviceUniqueIdUTF8_){
	return screenId_;
            deviceUniqueIdUTF8_ = NULL;
        }
        int nBufLen = strlen(deviceUniqueIdUTF8)+1;
        deviceUniqueIdUTF8_ = new char[nBufLen];
        memset(deviceUniqueIdUTF8_,0,nBufLen);
        
        memcpy(deviceUniqueIdUTF8_,deviceUniqueIdUTF8,strlen(deviceUniqueIdUTF8));
	return deviceNameUTF8_;
    }
    
    ScreenId    DesktopDisplayDevice::getScreenId()
    {
        return screenId_;
    }
    char*   DesktopDisplayDevice::getDeivceName()
    {
        return deviceNameUTF8_;
    }
    char*   DesktopDisplayDevice::getUniqueIdName()
    {
	return deviceUniqueIdUTF8_;
    }
    DesktopDisplayDevice& DesktopDisplayDevice::operator= (DesktopDisplayDevice& other)
    {
	screenId_ = other.getScreenId();
	setUniqueIdName(other.getUniqueIdName());
	setDeviceName(other.getDeviceName());
        
	return *this;
    }
    
    //================================================
    //
    DesktopApplication::DesktopApplication()
    {
	processId_ =0;
	processPathNameUTF8_= NULL;
	applicationNameUTF8_= NULL;
	processUniqueIdUTF8_= NULL;
    }
    DesktopApplication::~DesktopApplication()
    {
        
    }
    void DesktopApplication::setProcessId(const ProcessId processId){
	processId_ = processId;
    }
    void DesktopApplication::setProcessPathName(const char* appPathNameUTF8){
	SetStringMember(&processPathNameUTF8_, appPathNameUTF8);
        
        if(processPathNameUTF8_){
            delete []processPathNameUTF8_;
            processPathNameUTF8_ = NULL;
        }
        int nBufLen = strlen(appPathNameUTF8)+1;
        processPathNameUTF8_ = new char[nBufLen];
        memset(processPathNameUTF8_,0,nBufLen);
        
        memcpy(processPathNameUTF8_,appPathNameUTF8,strlen(appPathNameUTF8));
    }

    void DesktopApplication::setUniqueIdName(const char* appUniqueIdUTF8){
	SetStringMember(&processUniqueIdUTF8_, appUniqueIdUTF8);
        
        if(processPathNameUTF8_){
            delete []processPathNameUTF8_;
            processPathNameUTF8_ = NULL;
        }
        int nBufLen = strlen(appUniqueIdUTF8)+1;
        processUniqueIdUTF8_ = new char[nBufLen];
        memset(processUniqueIdUTF8_,0,nBufLen);
        
        memcpy(processUniqueIdUTF8_,appUniqueIdUTF8,strlen(appUniqueIdUTF8));
    }

    void DesktopApplication::setProcessAppName(const char* appNameUTF8){
	SetStringMember(&applicationNameUTF8_, appNameUTF8);
        
        if(applicationNameUTF8_){
            delete []applicationNameUTF8_;
            applicationNameUTF8_ = NULL;
        }
        int nBufLen = strlen(appNameUTF8)+1;
        applicationNameUTF8_ = new char[nBufLen];
        memset(applicationNameUTF8_,0,nBufLen);
        
        memcpy(applicationNameUTF8_,appNameUTF8,strlen(appNameUTF8));
    }

    
    ProcessId DesktopApplication::getProcessId(){
	return processId_;
    }

    char*  DesktopApplication::getProcessPathName(){
	return processPathNameUTF8_;
    }
    char*  DesktopApplication::getUniqueIdName(){
	return processUniqueIdUTF8_;
    }
    char*  DesktopApplication::getProcessAppName(){
	return applicationNameUTF8_;
    }
    DesktopApplication& DesktopApplication::operator= (DesktopApplication& other)
    {
	processId_ = other.getProcessId();
	setProcessPathName(other.getProcessPathName());
	setUniqueIdName(other.getUniqueIdName());
	setProcessAppName(other.getProcessAppName());
        
	return *this;
    }
    //================================================
    //
    DesktopDeviceInfoImpl::DesktopDeviceInfoImpl()
    {
        
    }
    DesktopDeviceInfoImpl::~DesktopDeviceInfoImpl()
    {
	CleanUp();
    
    }
    int32_t DesktopDeviceInfoImpl::Init()
    {
        //
        return Refresh();
    }
    
    int32_t DesktopDeviceInfoImpl::getDisplayDeviceCount()
	RefreshScreenList();
	return desktop_display_list_.size();
    }

    int32_t DesktopDeviceInfoImpl::getDesktopDisplayDeviceInfo(int32_t nIndex,DesktopDisplayDevice & desktopDisplayDevice)
	//Refresh();
	return 0;
}

int32_t DesktopDeviceInfoImpl::Refresh() {
	RefreshWindowList();
	RefreshScreenList();
	RefreshApplicationList();
	return 0;
				DesktopDisplayDevice & desktopDisplayDevice) {

	if(nIndex < 0 || nIndex >= desktop_display_list_.size()) {
		return -1;
	}
        
	std::map<intptr_t,DesktopDisplayDevice*>::iterator iter = desktop_display_list_.begin();
	std::advance (iter, nIndex);
	DesktopDisplayDevice * pDesktopDisplayDevice = iter->second;
	if(pDesktopDisplayDevice) {
		desktopDisplayDevice = (*pDesktopDisplayDevice);
	}
        
	return 0;
    }

    int32_t DesktopDeviceInfoImpl::getApplicationCount()
	RefreshWindowList();
	return desktop_window_list_.size();
	DesktopDisplayDevice &windowDevice) {
	if (nIndex < 0 || nIndex >= desktop_window_list_.size()) {
		return -1;
	}
	std::map<intptr_t, DesktopDisplayDevice *>::iterator itr = desktop_window_list_.begin();
	std::advance(itr, nIndex);
	DesktopDisplayDevice * pWindow = itr->second;
	if (!pWindow) {
		return -1;
	}
	windowDevice = (*pWindow);
	return 0;
	RefreshApplicationList();
	return desktop_application_list_.size();
    }

    int32_t DesktopDeviceInfoImpl::getApplicationInfo(int32_t nIndex,DesktopApplication & desktopApplication)
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
            if(pDesktopDisplayDevice){
            }
}

void DesktopDeviceInfoImpl::CleanUpWindowList(){
	std::map<intptr_t, DesktopDisplayDevice *>::iterator iterWindow;
	for (iterWindow = desktop_window_list_.begin(); iterWindow != desktop_window_list_.end(); iterWindow++) {
		DesktopDisplayDevice * pWindow = iterWindow->second;
		delete pWindow;
		iterWindow->second = NULL;
	}
	desktop_window_list_.clear();
}

void DesktopDeviceInfoImpl::CleanUpScreenList(){
	std::map<intptr_t,DesktopDisplayDevice*>::iterator iterDevice;
	for (iterDevice=desktop_display_list_.begin(); iterDevice!=desktop_display_list_.end(); iterDevice++){
		DesktopDisplayDevice * pDesktopDisplayDevice = iterDevice->second;
		delete pDesktopDisplayDevice;
		iterDevice->second = NULL;
	}
	desktop_display_list_.clear();
}

void DesktopDeviceInfoImpl::CleanUpApplicationList(){
	std::map<intptr_t,DesktopApplication*>::iterator iterApp;
	for (iterApp=desktop_application_list_.begin(); iterApp!=desktop_application_list_.end(); iterApp++){
		DesktopApplication * pDesktopApplication = iterApp->second;
            if(pDesktopApplication){
		delete pDesktopApplication;
            }
		iterApp->second = NULL;
	}
	desktop_application_list_.clear();
}
/*vagouzhou@gmail.com >> we need refactoring it 
should not get windows list from capturer.
should refactoring it by moving logic of GetWindowList from capturer to desktop_device_info_xplatform.
then capturer use the independent device info. 
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
			DesktopDisplayDevice *pWinDevice = new DesktopDisplayDevice;
			if (!pWinDevice) {
				continue;
			}
			pWinDevice->setScreenId(itr->id);
			pWinDevice->setDeviceName(itr->title.c_str());
			char idStr[64];
			_snprintf_s(idStr, sizeof(idStr), sizeof(idStr) - 1, "\\win\\%ld", pWinDevice->getScreenId());
			snprintf(idStr, BUFSIZ, "\\win\\%ld", pWinDevice->getScreenId());
			pWinDevice->setUniqueIdName(idStr);
			desktop_window_list_[pWinDevice->getScreenId()] = pWinDevice;
		}
	}
	return 0;
int32_t DesktopDeviceInfoImpl::RefreshScreenList(){
	CleanUpScreenList();
	//List all Windows
#if !defined(MULTI_MONITOR_SCREENSHARE)
	DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
	if (pDesktopDeviceInfo) {
		pDesktopDeviceInfo->setScreenId(0);
		pDesktopDeviceInfo->setDeviceName("Primary Monitor");
		pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#1");

		desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
	}
#endif
	return 0;
}
int32_t DesktopDeviceInfoImpl::RefreshApplicationList(){
	return 0;
}