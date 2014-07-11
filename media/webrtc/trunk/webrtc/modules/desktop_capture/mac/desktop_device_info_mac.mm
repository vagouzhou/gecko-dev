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
    int32_t DesktopDeviceInfoMac::Refresh()
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
        Str255 buffer;
        
        ProcessInfoRec InfoRec;
        InfoRec.processInfoLength = sizeof(ProcessInfoRec);
        InfoRec.processName = buffer;   	// set the buffer
#ifdef __LP64__
        FSRef theFsSpec;
        InfoRec.processAppRef = &theFsSpec;
#else
        FSSpec 	theFsSpec;
        InfoRec.processAppSpec = &theFsSpec;
#endif
        
        ProcessSerialNumber psn = {0,kNoProcess};
        while(::GetNextProcess(&psn) == noErr)
        {
            if(::GetProcessInformation(&psn, &InfoRec) == noErr)
            {
#ifdef __LP64__
                bool java = false;
                CFStringRef name = NULL;
                LSCopyDisplayNameForRef(&theFsSpec, &name);
                if(name)
                {
                    java = (CFStringCompare(name, CFSTR("\pjava"), 0) == kCFCompareEqualTo);
                    CFRelease(name);
                }
#else
                bool java = (PLstrcmp(theFsSpec.name , "\pjava") == 0);
#endif
                if( (InfoRec.processType == OSType('APPL')) // || (InfoRec.processType == OSType('FNDR'))
                   || (InfoRec.processType == OSType('APPC')) || (InfoRec.processType == OSType('APPD'))
                   || (InfoRec.processType == OSType('dfil')) || (InfoRec.processType == OSType('cdev'))
                   || (InfoRec.processType == OSType('BNDL')) || java )
                {
                    if(InfoRec.processMode & modeOnlyBackground)
                        continue;
                    
                    pid_t pid = 0;
                    GetProcessPID(&psn,&pid);
                    
                    DesktopApplication *pDesktopApplication = new DesktopApplication;
                    if(pDesktopApplication){
                        pDesktopApplication->setProcessId(pid);
                        NSRunningApplication *ra = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
                        if(ra){
                            [ra executableURL] ;
                            pDesktopApplication->setProcessAppName([ra.localizedName UTF8String]);
                            pDesktopApplication->setProcessPathName([[ra.executableURL absoluteString] UTF8String]);
                            std::ostringstream s;
                            s<<pid;
                            pDesktopApplication->setUniqueIdName(s.str().c_str());
                            
                            desktop_application_list_[pDesktopApplication->getProcessId()] = pDesktopApplication;
                        }
                    }
                }
                
            }
        }
        return 0;
    }
    
} //namespace webrtc