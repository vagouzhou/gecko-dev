#include "webrtc/modules/desktop_capture/x11/desktop_device_info_x11.h"

#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"

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
            pDesktopDeviceInfo->setDeivceName("All Monitors");
            
            std::ostringstream sUniqueIdName;
            sUniqueIdName<<kFullDesktopScreenId;
            pDesktopDeviceInfo->setUniqueIdName(sUniqueIdName.str().c_str());
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
              // Iterate in reverse order to return windows from front to back.
              ::Window app_window =children[num_children - 1 - i];//window_util_x11.GetApplicationWindow(children[num_children - 1 - i]);

              if (!app_window
            		  || window_util_x11.IsDesktopElement(app_window))
            	  continue;

              //filter

              //
              //Add one application
				DesktopApplication *pDesktopApplication = new DesktopApplication;
				if(pDesktopApplication){
					unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
					//process id
					pDesktopApplication->setProcessId(processId);

					//process path name
					char szFilePathName[256]={0};
					pDesktopApplication->setProcessPathName(szFilePathName);

					//application name
					std::string strAppName="";
					window_util_x11.GetWindowTitle(app_window, &strAppName);
					pDesktopApplication->setProcessAppName(strAppName.c_str());

					//setUniqueIdName
					std::ostringstream s;
					s<<processId;
					pDesktopApplication->setUniqueIdName(s.str().c_str());
					desktop_application_list_[processId] = pDesktopApplication;
				}

            }

            if (children)
              XFree(children);
          }

        return 0;
        
    }
    
} //namespace webrtc
