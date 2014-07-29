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
#include <string.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>

#include <algorithm>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_display.h"
#include "webrtc/modules/desktop_capture/x11/x_error_trap.h"
#include "webrtc/modules/desktop_capture/x11/x_server_pixel_buffer.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"

namespace webrtc {

	namespace {


		class AppCapturerLinux : public AppCapturer
									,public SharedXDisplay::XEventHandler{
		public:
			AppCapturerLinux(const DesktopCaptureOptions& options);
			virtual ~AppCapturerLinux();

			// AppCapturer interface.
			virtual bool GetAppList(AppList* apps) OVERRIDE;
			virtual bool SelectApp(ProcessId processId) OVERRIDE;
			virtual bool BringAppToFront() OVERRIDE;

			// DesktopCapturer interface.
			virtual void Start(Callback* callback) OVERRIDE;
			virtual void Capture(const DesktopRegion& region) OVERRIDE;

			// SharedXDisplay::XEventHandler interface.
			virtual bool HandleXEvent(const XEvent& event) OVERRIDE;

		protected:
			Display* display() { return x_display_->display(); }
			bool initXServerPixBuffer();
			::Window getCurrentRootWindow();
			void captureWebRTC(const DesktopRegion& region);
			void captureSample(const DesktopRegion& region);

		private:
			Callback* callback_;
			ProcessId selected_process_;
			XServerPixelBuffer x_server_pixel_buffer_;

			scoped_refptr<SharedXDisplay> x_display_;
			bool has_composite_extension_;

			::Window root_window_;
			DISALLOW_COPY_AND_ASSIGN(AppCapturerLinux);
		};

		AppCapturerLinux::AppCapturerLinux(const DesktopCaptureOptions& options)
			: callback_(NULL),
		      x_display_(options.x_display()),
		      root_window_(DefaultRootWindow(display())),
		      selected_process_(0) {
			int event_base, error_base, major_version, minor_version;
			  if (XCompositeQueryExtension(display(), &event_base, &error_base) &&
			      XCompositeQueryVersion(display(), &major_version, &minor_version) &&
			      // XCompositeNameWindowPixmap() requires version 0.2
			      (major_version > 0 || minor_version >= 2)) {
			    has_composite_extension_ = true;
			  } else {
			    LOG(LS_INFO) << "Xcomposite extension not available or too old.";
			  }

			  x_display_->AddEventHandler(ConfigureNotify, this);
		}

		AppCapturerLinux::~AppCapturerLinux() {
			x_server_pixel_buffer_.Release();
			x_display_->RemoveEventHandler(ConfigureNotify, this);
		}

		// AppCapturer interface.
		bool AppCapturerLinux::GetAppList(AppList* apps){
			return true;
		}
		bool AppCapturerLinux::SelectApp(ProcessId processId){
			selected_process_ = processId;
			getCurrentRootWindow();
			if (!initXServerPixBuffer()){
				selected_process_ = 0;
				return false;
			}

			return true;
		}
		bool AppCapturerLinux::BringAppToFront(){
			return true;
		}

		// DesktopCapturer interface.
		void AppCapturerLinux::Start(Callback* callback) {
			assert(!callback_);
			assert(callback);

			callback_ = callback;
		}
		void AppCapturerLinux::Capture(const DesktopRegion& region) {
			return captureWebRTC(region);
		}
		void AppCapturerLinux::captureWebRTC(const DesktopRegion& region) {
			x_display_->ProcessPendingXEvents();

			  if (!has_composite_extension_) {
			    // Without the Xcomposite extension we capture when the whole window is
			    // visible on screen and not covered by any other window. This is not
			    // something we want so instead, just bail out.
			    LOG(LS_INFO) << "No Xcomposite extension detected.";
			    callback_->OnCaptureCompleted(NULL);
			    return;
			  }

			  DesktopFrame* frame =
			      new BasicDesktopFrame(x_server_pixel_buffer_.window_size());

			  x_server_pixel_buffer_.Synchronize();
			  x_server_pixel_buffer_.CaptureRect(DesktopRect::MakeSize(frame->size()),
			                                     frame);

			  callback_->OnCaptureCompleted(frame);
		}

		void AppCapturerLinux::captureSample(const DesktopRegion& region) {
			//Capture screen

			//calculate app visual/foreground region

			//fill background

			//fill visual

			//fill foreground

			//event
		}

		bool AppCapturerLinux::HandleXEvent(const XEvent& event) {
		  if (event.type == ConfigureNotify) {
		    XConfigureEvent xce = event.xconfigure;
		    if (!DesktopSize(xce.width, xce.height).equals(
		            x_server_pixel_buffer_.window_size())) {
		    	initXServerPixBuffer();
		    	return true;
		    }
		  }
		  return false;
		}

		bool AppCapturerLinux::initXServerPixBuffer(){
			if (!x_server_pixel_buffer_.Init(display(), root_window_)) {
				LOG(LS_ERROR) << "Failed to initialize pixel buffer after resizing.";
				return false;
			}

			return true;
		}

		::Window AppCapturerLinux::getCurrentRootWindow(){
			::Window window;

			WindowUtilX11 window_util_x11(x_display_);
			int num_screens = XScreenCount(display());
			for (int screen = 0; screen < num_screens; ++screen) {
				::Window root_window = XRootWindow(display(), screen);
				::Window parent;
				::Window *children;
				unsigned int num_children;
				int status = XQueryTree(display(), root_window, &root_window, &parent,
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
						|| window_util_x11.IsDesktopElement(app_window)
						|| window_util_x11.GetWindowStatus(app_window) == WithdrawnState )
					continue;

					unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
					if(processId!=0 && processId==selected_process_){
						window = app_window;
						break;
					}
				}

				if (children)
					XFree(children);
			}
			root_window_ = DefaultRootWindow(display());
			return window;
		}

	}  // namespace

	// static
	AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
	//	if (!options.x_display())
	//		return NULL;
		return new AppCapturerLinux(options);
	}

}  // namespace webrtc
