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

		typedef struct {
			short x1, x2, y1, y2;
		} Box, BOX, BoxRec, *BoxPtr;
		typedef struct _XRegion {
			long size;
			long numRects;
			BOX *rects;
			BOX extents;
		} REGION;

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
			bool updateRegions();

			DesktopFrame* captureWebRTC(const DesktopRegion& region);
			DesktopFrame* captureSample(const DesktopRegion& region);

			void fillDesktopFrameRegionWithColor(DesktopFrame* pDesktopFrame,Region rgn, uint32_t color);

		private:
			Callback* callback_;
			ProcessId selected_process_;
			XServerPixelBuffer x_server_pixel_buffer_;

			scoped_refptr<SharedXDisplay> x_display_;
			bool has_composite_extension_;

			Region rgn_mask_;
			Region rgn_visual_;
			Region rgn_background_;

			::Window root_window_;
			DISALLOW_COPY_AND_ASSIGN(AppCapturerLinux);
			bool use_sample_mode_;
		};

		AppCapturerLinux::AppCapturerLinux(const DesktopCaptureOptions& options)
			: callback_(NULL),
		      x_display_(options.x_display()),
		      root_window_(DefaultRootWindow(display())),
		      selected_process_(0),
		      use_sample_mode_(1) {
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

			  rgn_mask_ = XCreateRegion();
			  rgn_visual_ = XCreateRegion();
			  rgn_background_ = XCreateRegion();
		}

		AppCapturerLinux::~AppCapturerLinux() {
			x_server_pixel_buffer_.Release();
			x_display_->RemoveEventHandler(ConfigureNotify, this);

			if (rgn_mask_) XDestroyRegion(rgn_mask_);
			if (rgn_visual_) XDestroyRegion(rgn_visual_);
			if (rgn_background_) XDestroyRegion(rgn_background_);
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
			DesktopFrame*  frame = NULL;
			if(use_sample_mode_)
				frame = captureSample(region);
			else
				frame = captureWebRTC(region);
			callback_->OnCaptureCompleted(frame);
			return ;
		}
		DesktopFrame* AppCapturerLinux::captureWebRTC(const DesktopRegion& region) {
			x_display_->ProcessPendingXEvents();

			  if (!has_composite_extension_) {
			    // Without the Xcomposite extension we capture when the whole window is
			    // visible on screen and not covered by any other window. This is not
			    // something we want so instead, just bail out.
			    LOG(LS_INFO) << "No Xcomposite extension detected.";
			    return NULL;
			  }

			  DesktopFrame* frame =
			      new BasicDesktopFrame(x_server_pixel_buffer_.window_size());

			  x_server_pixel_buffer_.Synchronize();
			  x_server_pixel_buffer_.CaptureRect(DesktopRect::MakeSize(frame->size()),
			                                     frame);

			  return frame;
		}

		DesktopFrame* AppCapturerLinux::captureSample(const DesktopRegion& region) {
			//Capture screen >> set root window as capture window
			DesktopFrame* frame = captureWebRTC(region);
			if(frame){
				//calculate app visual/foreground region
				updateRegions();

				//fill background with black
				fillDesktopFrameRegionWithColor(frame,rgn_background_,0x000000FF);

				//fill visual
				//do nothing

				//fill foreground with yellow
				fillDesktopFrameRegionWithColor(frame,rgn_mask_,0x00FFFFFF);

			}

			return frame;
		}

		void AppCapturerLinux::fillDesktopFrameRegionWithColor(DesktopFrame* pDesktopFrame,Region rgn, uint32_t color){
			if(!pDesktopFrame) return;
			if(XEmptyRegion(rgn)) return;

			REGION * st_rgn = (REGION *)rgn;
			if(st_rgn && st_rgn->numRects >0) {
				for(short i=0;i<st_rgn->numRects;i++){
					for(short j=st_rgn->rects[i].y1;j<st_rgn->rects[i].y2;j++){
					    uint32_t* dst_pos = reinterpret_cast<uint32_t*>(pDesktopFrame->data() + pDesktopFrame->stride() * j);
						for(short k=st_rgn->rects[i].x1;k<st_rgn->rects[i].x2;k++){
							dst_pos[k] = color;
						}
					}
				}
			}

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

			if(use_sample_mode_)
				return DefaultRootWindow(display());

			::Window window;

			WindowUtilX11 window_util_x11(x_display_);
			int num_screens = XScreenCount(display());
			for (int screen = 0; screen < num_screens; ++screen) {
				::Window root_window = XRootWindow(display(), screen);
				::Window parent;
				::Window root_return;
				::Window *children;
				unsigned int num_children;
				int status = XQueryTree(display(), root_window, &root_return, &parent,
										&children, &num_children);
				if (status == 0) {
					LOG(LS_ERROR) << "Failed to query for child windows for screen "
					<< screen;
					continue;
				}

				for (unsigned int i = 0; i < num_children; ++i) {
					::Window app_window =children[i];

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
			root_window_ = window;
			return window;
		}

		bool AppCapturerLinux::updateRegions(){

			WindowUtilX11 window_util_x11(x_display_);
			int num_screens = XScreenCount(display());
			for (int screen = 0; screen < num_screens; ++screen) {
				int nScreenCX = DisplayWidth (display(), screen);
				int nScreenCY = DisplayHeight (display(), screen);

			    XRectangle  screen_rect;
			    screen_rect.x =0;
			    screen_rect.y =0;
			    screen_rect.width = nScreenCX;
			    screen_rect.height = nScreenCY;

			    XUnionRectWithRegion(&screen_rect, rgn_background_, rgn_background_);
			    XXorRegion(rgn_mask_, rgn_mask_, rgn_mask_);
			    XXorRegion(rgn_visual_, rgn_visual_, rgn_visual_);

				::Window root_window = XRootWindow(display(), screen);
				::Window parent;
				::Window root_return;
				::Window *children;
				unsigned int num_children;
				int status = XQueryTree(display(), root_window, &root_return, &parent,
										&children, &num_children);
				if (status == 0) {
					LOG(LS_ERROR) << "Failed to query for child windows for screen "
					<< screen;
					continue;
				}

				for (unsigned int i = 0; i < num_children; ++i) {
					::Window app_window =children[i];

					if (!app_window
						|| window_util_x11.IsDesktopElement(app_window)
						|| window_util_x11.GetWindowStatus(app_window) == WithdrawnState )
					continue;

					//
					XWindowAttributes win_info;
					if (!XGetWindowAttributes(display(), app_window, &win_info)){
						continue;
					}
					if (win_info.c_class != InputOutput || win_info.map_state != IsViewable){
						continue;
					}

					int absx,absy;
					::Window temp_win;
					if (!XTranslateCoordinates(display(),app_window,root_return, 0,0,&absx,&absy,&temp_win)){
						continue;
					}
					if (absx < 0){
						win_info.width += absx;
						absx = 0;
					}
					else if((absx+win_info.width)>nScreenCX){
						win_info.width= nScreenCX - absx;
					}
					if (absy < 0){
						win_info.height += absy;
						absy = 0;
					}
					else if((absy + win_info.height)>nScreenCY){
						win_info.height = nScreenCY - absy;
					}
					if(win_info.width<=0 || win_info.height <=0 )
						continue;
					Region win_rgn = XCreateRegion();
					XRectangle  win_rect;
					win_rect.x =absx;
					win_rect.y =absy;
					win_rect.width = win_info.width;
					win_rect.height = win_info.height;
					XUnionRectWithRegion(&win_rect, win_rgn, win_rgn);


					unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
					if(processId!=0 && processId==selected_process_){
						XUnionRegion(win_rgn,rgn_visual_,rgn_visual_);
						XSubtractRegion(rgn_mask_,win_rgn,rgn_mask_);
					}
					else{
						Region win_rgn_intersect = XCreateRegion();
						XIntersectRegion(win_rgn,rgn_visual_,win_rgn_intersect);
						XUnionRegion(win_rgn_intersect,rgn_mask_,rgn_mask_);
						if(win_rgn_intersect)
							XDestroyRegion(win_rgn_intersect);

						XSubtractRegion(rgn_visual_,win_rgn,rgn_visual_);
					}
					if(win_rgn)
						XDestroyRegion(win_rgn);
				}

				if (children)
					XFree(children);
			}

			XSubtractRegion(rgn_background_,rgn_visual_,rgn_background_);
			return true;
		}

	}  // namespace

	// static
	AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
	//	if (!options.x_display())
	//		return NULL;
		return new AppCapturerLinux(options);
	}

}  // namespace webrtc
