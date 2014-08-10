#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DEVICE_INFO_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DEVICE_INFO_H_

#include "webrtc/typedefs.h"
#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc {
    class DesktopDeviceInfoMac : public DesktopDeviceInfoImpl{
    public:
        DesktopDeviceInfoMac();
        ~DesktopDeviceInfoMac();
        
  //DesktopDeviceInfoImpl Interfaces
  virtual int32_t RefreshApplicationList();
    };
}// namespace webrtc
#endif //WEBRTC_MODULES_DESKTOP_CAPTURE_MAC_DEVICE_INFO_H_