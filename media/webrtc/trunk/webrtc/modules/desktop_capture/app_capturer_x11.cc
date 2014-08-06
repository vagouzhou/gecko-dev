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
#include "webrtc/modules/desktop_capture/screen_capturer.h"
#include "webrtc/modules/desktop_capture/shared_desktop_frame.h"

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
		class WindowsCapturerProxy : DesktopCapturer::Callback {
		public:
			WindowsCapturerProxy():window_capturer_(WindowCapturer::Create()){
				window_capturer_->Start(this);
			}
			~WindowsCapturerProxy(){}
			void SelectWindow(WindowId windowId)  { window_capturer_->SelectWindow(windowId); }
			scoped_ptr<DesktopFrame>& GetFrame() { return frame_; }
			void Capture(const DesktopRegion& region){ window_capturer_->Capture(region); }

			// Callback interface
			virtual SharedMemory *CreateSharedMemory(size_t) OVERRIDE { return NULL; }
			virtual void OnCaptureCompleted(DesktopFrame *frame) OVERRIDE{frame_.reset(frame);}

		private:
			scoped_ptr<WindowCapturer> window_capturer_;
			scoped_ptr<DesktopFrame> frame_;
		};

		class ScreenCapturerProxy: DesktopCapturer::Callback{
		public:
			ScreenCapturerProxy():screen_capturer_(ScreenCapturer::Create()){
				screen_capturer_->SelectScreen(kFullDesktopScreenId);
				screen_capturer_->Start(this);
			}
			void Capture(const DesktopRegion& region){ screen_capturer_->Capture(region); }
			scoped_ptr<DesktopFrame>& GetFrame() { return frame_; }
			// Callback interface
			virtual SharedMemory *CreateSharedMemory(size_t) OVERRIDE { return NULL; }
			virtual void OnCaptureCompleted(DesktopFrame *frame) OVERRIDE{frame_.reset(frame);}
		protected:
			scoped_ptr<ScreenCapturer> screen_capturer_;
			scoped_ptr<DesktopFrame> frame_;
		};

		class AppCapturerLinux : public AppCapturer{
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

		protected:
			Display* display() { return x_display_->display(); }
			::Window getCurrentRootWindow();
			bool updateRegions();
			//Debug
			void printRegion(Region rgn);
			void printWindow(::Window window);

			void captureWebRTC(const DesktopRegion& region);
			void captureSample(const DesktopRegion& region);

			void fillDesktopFrameRegionWithColor(DesktopFrame* pDesktopFrame,Region rgn, uint32_t color);

		private:
			Callback* callback_;
			ProcessId selected_process_;

			//Sample Mode
			ScreenCapturerProxy screen_capturer_proxy_;
			Region rgn_mask_;
			Region rgn_visual_;
			Region rgn_background_;

			//WebRtc Window mode
			WindowsCapturerProxy window_capturer_proxy_;

			scoped_refptr<SharedXDisplay> x_display_;
			DISALLOW_COPY_AND_ASSIGN(AppCapturerLinux);
		};

		AppCapturerLinux::AppCapturerLinux(const DesktopCaptureOptions& options)
			: callback_(NULL),
		      x_display_(options.x_display()),
		      selected_process_(0){
			  rgn_mask_ = XCreateRegion();
			  rgn_visual_ = XCreateRegion();
			  rgn_background_ = XCreateRegion();
		}

		AppCapturerLinux::~AppCapturerLinux() {

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
			captureSample(region);
			//captureWebRTC(region);
			return;
		}
		void AppCapturerLinux::captureWebRTC(const DesktopRegion& region) {
			//DesktopFrame* frame = new BasicDesktopFrame(x_server_pixel_buffer_.window_size());
			//
			window_capturer_proxy_.SelectWindow(getCurrentRootWindow());
			window_capturer_proxy_.Capture(region);
			//trigger event
			if(callback_)
				callback_->OnCaptureCompleted(window_capturer_proxy_.GetFrame().release());
		}

		void AppCapturerLinux::captureSample(const DesktopRegion& region) {
			//Capture screen >> set root window as capture window
			screen_capturer_proxy_.Capture(region);
			DesktopFrame* frame = screen_capturer_proxy_.GetFrame().get();
			if(frame){
				//calculate app visual/foreground region
				updateRegions();

				//fill background with black
				fillDesktopFrameRegionWithColor(frame,rgn_background_,0xFF000000);

				//fill visual
				//do nothing

				//fill foreground with yellow
				fillDesktopFrameRegionWithColor(frame,rgn_mask_,0xFFFFFF00);

			}

			//trigger event
			if(callback_)
				callback_->OnCaptureCompleted(screen_capturer_proxy_.GetFrame().release());
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

		::Window AppCapturerLinux::getCurrentRootWindow(){
			//test >> return DefaultRootWindow(display());
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
			//root_window_= DefaultRootWindow(display());
			return window;
		}

		void AppCapturerLinux::printRegion(Region rgn){
			LOG(LS_INFO) << "AppCapturerLinux::printRegion";
			REGION * st_rgn = (REGION *)rgn;
			if(st_rgn && st_rgn->numRects >0) {
				for(short i=0;i<st_rgn->numRects;i++){
					LOG(LS_INFO) << "box index=" << i
										<< "x1=" << st_rgn->rects[i].x1
										<< "y1="<< st_rgn->rects[i].y1
										<< "x2="<< st_rgn->rects[i].x2
										<< "y2="<< st_rgn->rects[i].y2;
				}
			}
		}

		void AppCapturerLinux::printWindow(::Window window){

			WindowUtilX11 window_util_x11(x_display_);
			unsigned int processId = window_util_x11.GetWindowProcessID(window);
			std::string strAppName="";
			window_util_x11.GetWindowTitle(window, &strAppName);
			int32_t nStatus =window_util_x11.GetWindowStatus(window);
			XRectangle  win_rect;
			window_util_x11.GetWindowRect(window,win_rect);

			LOG(LS_INFO) << "AppCapturerLinux::printWindow"
					<< "processId=" << processId
					<< "strAppName=" << strAppName
					<< "win_rect:" << win_rect.x <<","<< win_rect.y<<"," << win_rect.width <<","<< win_rect.height
					<< "nStatus=" << nStatus;

		}
		bool AppCapturerLinux::updateRegions(){
			XSubtractRegion(rgn_visual_,rgn_visual_,rgn_visual_);
			XSubtractRegion(rgn_mask_,rgn_mask_,rgn_mask_);
			/*
			//test fillDesktopFrameRegionWithColor >>
			{
				int nScreenCX = DisplayWidth (display(), 0);
				int nScreenCY = DisplayHeight (display(), 0);

				XRectangle  screen_rect;
				screen_rect.x =0;
				screen_rect.y =0;
				screen_rect.width = nScreenCX;
				screen_rect.height = nScreenCY;

				XUnionRectWithRegion(&screen_rect, rgn_background_, rgn_background_);

				//test>>
				XRectangle  rect_test;
				rect_test.x =0;
				rect_test.y =0;
				rect_test.width = 500;
				rect_test.height = 400;
				XUnionRectWithRegion(&rect_test, rgn_visual_, rgn_visual_);


				rect_test.x =100;
				rect_test.y =100;
				rect_test.width = 100;
				rect_test.height = 100;
				XUnionRectWithRegion(&rect_test,rgn_mask_,rgn_mask_);

				XSubtractRegion(rgn_background_,rgn_visual_,rgn_background_);

				return true;
			}
			//<< test fillDesktopFrameRegionWithColor end
			*/

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

					//filter
					/*
					if (!app_window
						|| window_util_x11.IsDesktopElement(app_window)
						|| window_util_x11.GetWindowStatus(app_window) == WithdrawnState )
					continue;


					XWindowAttributes win_info;
					if (!XGetWindowAttributes(display(), app_window, &win_info)){
						continue;
					}
					if (win_info.c_class != InputOutput || win_info.map_state != IsViewable){
						continue;
					}
					*/
					//Debug for windows info
					printWindow(app_window);

					//Get window region
					Region win_rgn = XCreateRegion();
					XRectangle  win_rect;
					window_util_x11.GetWindowRect(app_window,win_rect);
					XUnionRectWithRegion(&win_rect, win_rgn, win_rgn);
					if(win_rect.width <=0 || win_rect.height <=0) continue;

					//update rgn_visual_ , rgn_mask_,
					unsigned int processId = window_util_x11.GetWindowProcessID(app_window);
					if(processId!=0 && processId==selected_process_){
						XUnionRegion(rgn_visual_,win_rgn,rgn_visual_);
						XSubtractRegion(rgn_mask_,win_rgn,rgn_mask_);

						printRegion(rgn_mask_);
						printRegion(rgn_visual_);
					}
					else{
						Region win_rgn_intersect = XCreateRegion();
						XIntersectRegion(rgn_visual_,win_rgn,win_rgn_intersect);

						printRegion(win_rgn_intersect);
						printRegion(rgn_mask_);
						printRegion(rgn_visual_);

						XSubtractRegion(rgn_visual_,win_rgn_intersect,rgn_visual_);
						XUnionRegion(win_rgn_intersect,rgn_mask_,rgn_mask_);

						printRegion(win_rgn_intersect);
						printRegion(rgn_mask_);
						printRegion(rgn_visual_);

						if(win_rgn_intersect)
							XDestroyRegion(win_rgn_intersect);
					}
					if(win_rgn)
						XDestroyRegion(win_rgn);
				}

				if (children)
					XFree(children);
			}

			XSubtractRegion(rgn_background_,rgn_visual_,rgn_background_);
			printRegion(rgn_mask_);
			printRegion(rgn_visual_);
			printRegion(rgn_background_);

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
