#include "webrtc/modules/desktop_capture/mac/desktop_device_info_win.h"

namespace webrtc{

#define MULTI_MONITOR_NO_SUPPORT 1
    
DesktopDeviceInfo * DesktopDeviceInfoImpl::Create()
{
    DesktopDeviceInfoWin * pDesktopDeviceInfo = new DesktopDeviceInfoWin();
    if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init()!=0){
        delete pDesktopDeviceInfo;
        pDesktopDeviceInfo = NULL;
    }
    return pDesktopDeviceInfo;
}

DesktopDeviceInfoWin::DesktopDeviceInfoWin()
{
    
}
DesktopDeviceInfoWin:DesktopDeviceInfoWin()
{
    
}
    
int32_t DesktopDeviceInfoWin::Init()
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
