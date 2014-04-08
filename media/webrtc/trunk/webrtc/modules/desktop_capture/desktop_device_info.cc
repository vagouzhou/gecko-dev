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
    void DesktopDisplayDevice::setDeivceName(char* deviceNameUTF8)
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
    void DesktopDisplayDevice::setUniqueIdName(char* deviceUniqueIdUTF8)
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
        
    }
    DesktopApplication::~DesktopApplication()
    {
        
    }

    
    //================================================
    //
    DesktopDeviceInfoImpl::DesktopDeviceInfoImpl()
    {
        
    }
    DesktopDeviceInfoImpl::~DesktopDeviceInfoImpl()
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
    
    int32_t DesktopDeviceInfoImpl::getDisplayDeviceCount()
    {
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
        return desktop_application_list_.size();
    }

    int32_t DesktopDeviceInfoImpl::getApplicationInfo(int32_t nIndex,DesktopApplication & desktopApplication)
    {
        return 0;
    }

}