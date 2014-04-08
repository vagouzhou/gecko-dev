#include "webrtc/modules/desktop_capture/mac/desktop_device_info_mac.h"
#include <Cocoa/Cocoa.h>

namespace webrtc {
    
#define MULTI_MONITOR_NO_SUPPORT 1
DesktopDeviceInfo * DesktopDeviceInfoImpl::Create()
{
    DesktopDeviceInfoMac * pDesktopDeviceInfo = new DesktopDeviceInfoMac();
    if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init()!=0){
        delete pDesktopDeviceInfo;
        pDesktopDeviceInfo = NULL;
    }
    return pDesktopDeviceInfo;
}
DesktopDeviceInfoMac::DesktopDeviceInfoMac()
{
    
}
DesktopDeviceInfoMac::~DesktopDeviceInfoMac()
{
    
}    
int32_t DesktopDeviceInfoMac::Init()
{
#ifdef MULTI_MONITOR_NO_SUPPORT
    DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
    if(pDesktopDeviceInfo){
        pDesktopDeviceInfo->setScreenId(0);
        pDesktopDeviceInfo->setDeivceName("Primary Monitor");
        pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#1");
        
        desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
    }
#endif
    //
    
    return 0;
}
    
} //namespace webrtc