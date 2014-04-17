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
