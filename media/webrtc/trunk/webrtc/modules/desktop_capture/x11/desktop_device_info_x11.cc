#include "webrtc/modules/desktop_capture/x11/desktop_device_info_x11.h"

namespace webrtc{
#define MULTI_MONITOR_NO_SUPPORT 1
    DesktopDeviceInfo * DesktopDeviceInfoImpl::Create()
    {
        DesktopDeviceInfoX11 * pDesktopDeviceInfo = new DesktopDeviceInfoX11();
        if(pDesktopDeviceInfo && pDesktopDeviceInfo->Init()!=0){
            delete pDesktopDeviceInfo;
            pDesktopDeviceInfo = NULL;
        }
        return pDesktopDeviceInfo;
    }
    DesktopDeviceInfoX11::DesktopDeviceInfoX11()
    {
        
    }
    DesktopDeviceInfoX11::~DesktopDeviceInfoX11()
    {
        
    }
    
    int32_t DesktopDeviceInfoX11::Refresh()
    {
        //Clean up sources first
        CleanUp();
        
        //List display
#ifdef MULTI_MONITOR_NO_SUPPORT
        DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
        if(pDesktopDeviceInfo){
            pDesktopDeviceInfo->setScreenId(kFullDesktopScreenId);
            pDesktopDeviceInfo->setDeivceName("Screen");
            pDesktopDeviceInfo->setUniqueIdName("\\screen\\all");
            
            desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
        }
#else
        /*
         int nCountScreen = 0;
         
         for(int i=0;i<nCountScreen;i++){
         DesktopDisplayDevice *pDesktopDeviceInfo = new DesktopDisplayDevice;
         if(pDesktopDeviceInfo){
         pDesktopDeviceInfo->setScreenId(0);
         pDesktopDeviceInfo->setDeivceName("Monitor Name: XXX");
         pDesktopDeviceInfo->setUniqueIdName("\\screen\\monitor#x");
         
         desktop_display_list_[pDesktopDeviceInfo->getScreenId()] = pDesktopDeviceInfo;
         }
         }
         */
#endif
        
        //List all running applicatones exclude background process.
        
        
        return 0;
        
    }
    
} //namespace webrtc