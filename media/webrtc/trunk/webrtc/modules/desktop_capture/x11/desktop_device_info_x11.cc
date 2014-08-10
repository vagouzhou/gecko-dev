/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "webrtc/modules/desktop_capture/x11/desktop_device_info_x11.h"
#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"

namespace webrtc{

DesktopDeviceInfo * DesktopDeviceInfoImpl::Create() {
	DesktopDeviceInfoX11 * pDesktopDeviceInfo = new DesktopDeviceInfoX11();
	if (pDesktopDeviceInfo && pDesktopDeviceInfo->Init() != 0){
		delete pDesktopDeviceInfo;
		pDesktopDeviceInfo = NULL;
	}
	return pDesktopDeviceInfo;
}

DesktopDeviceInfoX11::DesktopDeviceInfoX11() {
}

DesktopDeviceInfoX11::~DesktopDeviceInfoX11() {
}

int32_t DesktopDeviceInfoX11::RefreshApplicationList() {
	//Clean up sources first
	CleanUpApplicationList();

	//List all running applications exclude background process.
	scoped_refptr<SharedXDisplay> SharedDisplay = SharedXDisplay::CreateDefault();
	XErrorTrap error_trap(SharedDisplay->display());

	WindowUtilX11 window_util_x11(SharedDisplay);
	int num_screens = XScreenCount(SharedDisplay->display());
	for (int screen = 0; screen < num_screens; ++screen) {
		::Window root_window = XRootWindow(SharedDisplay->display(), screen);
		::Window parent;
		::Window *children;
		unsigned int num_children;
		int status = XQueryTree(SharedDisplay->display(), root_window, &root_window, &parent,
			&children, &num_children);
		if (status == 0) {
			LOG(LS_ERROR) << "Failed to query for child windows for screen "
				<< screen;
			continue;
		}

		for (unsigned int i = 0; i < num_children; ++i) {
			::Window app_window =window_util_x11.GetApplicationWindow(children[num_children - 1 - i]);//window_util_x11.GetClientWindow(children[i]);//

			if (!app_window
				|| window_util_x11.IsDesktopElement(app_window)
				|| window_util_x11.GetWindowStatus(app_window) == WithdrawnState )
				continue;

			//what application want to filter out ?

			//

			unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
			if(processId!=0){
				//Add one application
				DesktopApplication *pDesktopApplication = new DesktopApplication;
				if(pDesktopApplication){
					//process id
					pDesktopApplication->setProcessId(processId);

					//process path name
					char szFilePathName[256]={0};
					pDesktopApplication->setProcessPathName(szFilePathName);

					//application name
					std::string strAppName="";
					window_util_x11.GetWindowTitle(app_window, &strAppName);
					pDesktopApplication->setDeviceName(strAppName.c_str());

					//setUniqueIdName
					std::ostringstream s;
					s<<processId;
					pDesktopApplication->setUniqueIdName(s.str().c_str());
					desktop_application_list_[processId] = pDesktopApplication;
				}
			}

		}

		if (children)
			XFree(children);
	}

	return 0;
}

} //namespace webrtc
