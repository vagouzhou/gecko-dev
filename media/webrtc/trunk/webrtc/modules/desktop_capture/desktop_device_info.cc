#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc{
    
    //================================================
    //
    DesktopDisplayDevice::DesktopDisplayDevice(){
        screenId_ = kInvalidScreenId;
        deviceUniqueIdUTF8_ = NULL;
        deviceNameUTF8_ = NULL;
    }
    DesktopDisplayDevice::~DesktopDisplayDevice(){
        screenId_ = kInvalidScreenId;
        
        if(deviceUniqueIdUTF8_){
            delete []deviceUniqueIdUTF8_;
        }
        if(deviceNameUTF8_){
            delete []deviceNameUTF8_;
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
            delete []deviceNameUTF8_;
            deviceNameUTF8_ = NULL;
        }
        int nBufLen = strlen(deviceNameUTF8)+1;
        deviceNameUTF8_ = new char[nBufLen];
        memset(deviceNameUTF8_,0,nBufLen);
        
        memcpy(deviceNameUTF8_,deviceNameUTF8,strlen(deviceNameUTF8));
    }
    void DesktopDisplayDevice::setUniqueIdName(const char* deviceUniqueIdUTF8)
    {
        if(deviceUniqueIdUTF8==NULL) return;
        
        if(deviceUniqueIdUTF8_){
            delete []deviceUniqueIdUTF8_;
            deviceUniqueIdUTF8_ = NULL;
        }
        int nBufLen = strlen(deviceUniqueIdUTF8)+1;
        deviceUniqueIdUTF8_ = new char[nBufLen];
        memset(deviceUniqueIdUTF8_,0,nBufLen);
        
        memcpy(deviceUniqueIdUTF8_,deviceUniqueIdUTF8,strlen(deviceUniqueIdUTF8));
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
        setDeivceName(other.getDeivceName());
        
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
        if(appPathNameUTF8==NULL) return;
        
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
        if(appUniqueIdUTF8==NULL) return;
        
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
        if(appNameUTF8==NULL) return;
        
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
    {
        Refresh();
        return desktop_display_list_.size();
    }
    
    int32_t DesktopDeviceInfoImpl::getDesktopDisplayDeviceInfo(int32_t nIndex,DesktopDisplayDevice & desktopDisplayDevice)
    {
        if(nIndex<0 || nIndex>=desktop_display_list_.size())
            return -1;
        
        std::map<intptr_t,DesktopDisplayDevice*>::iterator iter = desktop_display_list_.begin();
        std::advance (iter,nIndex);
        DesktopDisplayDevice * pDesktopDisplayDevice = iter->second;
        if(pDesktopDisplayDevice)
            desktopDisplayDevice = (*pDesktopDisplayDevice);
        
        return 0;
    }

    int32_t DesktopDeviceInfoImpl::getApplicationCount()
    {
        Refresh();
        return desktop_application_list_.size();
    }

    int32_t DesktopDeviceInfoImpl::getApplicationInfo(int32_t nIndex,DesktopApplication & desktopApplication)
    {
        if(nIndex<0 || nIndex>=desktop_application_list_.size())
            return -1;
        
        std::map<intptr_t,DesktopApplication*>::iterator iter = desktop_application_list_.begin();
        std::advance (iter,nIndex);
        DesktopApplication * pDesktopApplication = iter->second;
        if(pDesktopApplication)
            desktopApplication = (*pDesktopApplication);        

        return 0;
    }
    void DesktopDeviceInfoImpl::CleanUp()
    {   
        //
        std::map<intptr_t,DesktopDisplayDevice*>::iterator iterDevice;
        for(iterDevice=desktop_display_list_.begin();iterDevice!=desktop_display_list_.end();iterDevice++){
            DesktopDisplayDevice * pDesktopDisplayDevice = iterDevice->second;
            if(pDesktopDisplayDevice){
                delete pDesktopDisplayDevice;
            }
            iterDevice->second = NULL;
        }
        desktop_display_list_.clear();
        
        //
        std::map<intptr_t,DesktopApplication*>::iterator iterApp;
        for(iterApp=desktop_application_list_.begin();iterApp!=desktop_application_list_.end();iterApp++){
            DesktopApplication * pDesktopApplication = iterApp->second;
            if(pDesktopApplication){
                delete pDesktopApplication;
            }
            iterApp->second = NULL;
        }
        desktop_application_list_.clear();
    }
}