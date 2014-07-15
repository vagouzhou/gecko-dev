#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_DEVICE_INFO_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_DEVICE_INFO_H_

#include "webrtc/typedefs.h"
#include "webrtc/modules/desktop_capture/desktop_device_info.h"

namespace webrtc {
    class DesktopDeviceInfoX11 : public DesktopDeviceInfoImpl{
    public:
        DesktopDeviceInfoX11();
        ~DesktopDeviceInfoX11();
        
        //DesktopDeviceInfo Interfaces
        virtual int32_t Refresh();
    protected:

    };
}// namespace webrtc
#endif //WEBRTC_MODULES_DESKTOP_CAPTURE_X11_DEVICE_INFO_H_
