/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/desktop_capture/win/desktop_device_info_win.h"

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
    DesktopDeviceInfoWin::~DesktopDeviceInfoWin()
    {
        
    }
    
    int32_t DesktopDeviceInfoWin::Refresh()
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
