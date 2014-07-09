/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/desktop_capture/window_capturer.h"
#include "webrtc/modules/desktop_capture/app_capturer.h"

#include <assert.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "CGSPrivate.h"

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <vector>

namespace webrtc {

namespace {

class AppCapturerMac : public AppCapturer {
 public:
  AppCapturerMac();
  virtual ~AppCapturerMac();

	// AppCapturer interface.
	virtual bool GetAppList(AppList* apps);
	virtual bool SelectApp(ProcessId processId);
	virtual bool BringAppToFront();

	// DesktopCapturer interface.
	virtual void Start(Callback* callback) OVERRIDE;
	virtual void Capture(const DesktopRegion& region) OVERRIDE;
protected:
    
private:
	Callback* callback_;
	ProcessId process_id_;
    CGSConnection curCaptureConnection;
    
	DISALLOW_COPY_AND_ASSIGN(AppCapturerMac);
};

AppCapturerMac::AppCapturerMac()
    : callback_(NULL),
      process_id_(0) ,
        curCaptureConnection(0){
}

AppCapturerMac::~AppCapturerMac() {
}

// AppCapturer interface.
bool AppCapturerMac::GetAppList(AppList* apps){
    //TBD, we should has centrelized application , display source enumerate ,not implement it in capturer, it is useless
	return true;
}
bool AppCapturerMac::SelectApp(ProcessId processId){
    
    process_id_ = processId;
	
    //Selected Process Connection
    curCaptureConnection =0;
    CGSConnection defaultConnection_ = _CGSDefaultConnection();
    ProcessSerialNumber curCaptureProcessSerialNumber;
    GetProcessForPID(process_id_,&curCaptureProcessSerialNumber);
    CGSGetConnectionIDForPSN(defaultConnection_, &curCaptureProcessSerialNumber, &curCaptureConnection);
    
	return true;
}

bool AppCapturerMac::BringAppToFront() {
	return true;
}

// DesktopCapturer interface.
void AppCapturerMac::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;
}
void AppCapturerMac::Capture(const DesktopRegion& region) {
    
    //Check selected process is exist
    if(curCaptureConnection == 0){
        callback_->OnCaptureCompleted(NULL);
        return ;
    }
    
    //Enumerate all windows
    CGSConnection defaultConnection_ = _CGSDefaultConnection();
	int windowCount;
	CGSGetOnScreenWindowCount(defaultConnection_, 0, &windowCount);
	int* windowList = new int[windowCount];
	CGSGetOnScreenWindowList(defaultConnection_, 0, windowCount, windowList, &windowCount);
	
	
    //Filter out windows not belong to current selected process
	CGWindowID* captureWindowList = new CGWindowID[windowCount];
    memset(captureWindowList,0,sizeof(CGWindowID)*(windowCount));
	int captureWindowListCount = 0;
    int i = 0;
	for(i = 0; i <windowCount; i++ ) //from back to front
	{
        CGSConnection winConnection = 0;
		CGSGetWindowOwner(defaultConnection_,windowList[i],&winConnection);
        
		if(winConnection == curCaptureConnection)
        {
            captureWindowList[captureWindowListCount++] = windowList[i];
        }
	}
    
    //Check windows list is not empty.
    if(captureWindowListCount<=0){
        delete []windowList;
        delete []captureWindowList;
        callback_->OnCaptureCompleted(NULL);
        return ;
    }
    
    //Don't Support multi-display yet.
    CGRect rectCapturedDisplay = CGDisplayBounds(CGMainDisplayID());

    //Capture all windows of selected process.
    CFArrayRef windowIDsArray = CFArrayCreate(kCFAllocatorDefault,
                                              (const void**)captureWindowList,
                                              captureWindowListCount,
                                              NULL);
	CGImageRef app_image = CGWindowListCreateImageFromArray(rectCapturedDisplay,
                                                            windowIDsArray,
                                                            kCGWindowImageDefault);
	CFRelease (windowIDsArray);
    delete []windowList;
    delete []captureWindowList;

    
    //Debug for capture raw data
    {
        static int iFile =0;
        if(iFile>=200)iFile = 0;
        NSString *filePathName = [NSString stringWithFormat:@"/tmp/gecko/%d_dump.png", ++iFile];
        CFURLRef url = (CFURLRef)[NSURL fileURLWithPath:filePathName];
        CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, NULL);
        CGImageDestinationAddImage(destination, app_image, nil);
        
        CGImageDestinationFinalize(destination);
        CFRelease(destination);
    }
    
    //Wrapper raw data into DesktopFrame
    if (!app_image) {
        CFRelease(app_image);
        callback_->OnCaptureCompleted(NULL);
        return;
    }
    
    int bits_per_pixel = CGImageGetBitsPerPixel(app_image);
    if (bits_per_pixel != 32) {
        LOG(LS_ERROR) << "Unsupported window image depth: " << bits_per_pixel;
        CFRelease(app_image);
        callback_->OnCaptureCompleted(NULL);
        return;
    }
    
    int width = CGImageGetWidth(app_image);
    int height = CGImageGetHeight(app_image);
    DesktopFrame* frame = new BasicDesktopFrame(DesktopSize(width, height));
    
    CGDataProviderRef provider = CGImageGetDataProvider(app_image);
    CFDataRef cf_data = CGDataProviderCopyData(provider);
    int src_stride = CGImageGetBytesPerRow(app_image);
    const uint8_t* src_data = CFDataGetBytePtr(cf_data);
    for (int y = 0; y < height; ++y) {
        memcpy(frame->data() + frame->stride() * y, src_data + src_stride * y,
               DesktopFrame::kBytesPerPixel * width);
    }
    
    CFRelease(cf_data);
    CFRelease(app_image);
    
    //Trigger callback event to up layer
    callback_->OnCaptureCompleted(frame);
}

}  // namespace

// static
AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
  return new AppCapturerMac();
}

}  // namespace webrtc
