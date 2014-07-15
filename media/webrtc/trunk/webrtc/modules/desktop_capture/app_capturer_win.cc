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
#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_frame_win.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

#include <vector>

namespace webrtc {

	namespace {
		typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL* enabled);

		class AppCapturerWin : public AppCapturer {
		public:
			AppCapturerWin();
			virtual ~AppCapturerWin();

			// AppCapturer interface.
			virtual bool GetAppList(AppList* apps);
			virtual bool SelectApp(ProcessId processId);
			virtual bool BringAppToFront();

			// DesktopCapturer interface.
			virtual void Start(Callback* callback) OVERRIDE;
			virtual void Capture(const DesktopRegion& region) OVERRIDE;
			struct WindowItem{
				HWND hWnd;
				RECT rcWindow;
				bool bSizeChanged;
			};
			struct EnumWindowsLPARAM {
				ProcessId process_id;
				std::vector<WindowItem> vecWindows;
			};
			static BOOL CALLBACK EnumWindowsProc(HWND handle, LPARAM lParam);
		protected:
			void CaptureByWebRTC(const DesktopRegion& region);
			void CaptureBySample(const DesktopRegion& region);
			void CaptureByMagnifier(const DesktopRegion& region);
		private:
			Callback* callback_;
			ProcessId processId_;

			std::vector<WindowItem> vec_previous_windows_;

			//DesktopRect rcDesktop_;
			//HDC memDcCapture_;
			//HBITMAP bmpCapture_;

			bool IsAeroEnabled();
			// dwmapi.dll is used to determine if desktop compositing is enabled.
			HMODULE dwmapi_library_;
			DwmIsCompositionEnabledFunc is_composition_enabled_func_;

			DISALLOW_COPY_AND_ASSIGN(AppCapturerWin);
		};

		AppCapturerWin::AppCapturerWin()
			: callback_(NULL),
			processId_(NULL){
				//memDcCapture_ = NULL;
				//bmpCapture_ = NULL;
				// Try to load dwmapi.dll dynamically since it is not available on XP.
				dwmapi_library_ = LoadLibrary(L"dwmapi.dll");
				if (dwmapi_library_) {
					is_composition_enabled_func_ =
						reinterpret_cast<DwmIsCompositionEnabledFunc>(
						GetProcAddress(dwmapi_library_, "DwmIsCompositionEnabled"));
					assert(is_composition_enabled_func_);
				} else {
					is_composition_enabled_func_ = NULL;
				}
		}

		AppCapturerWin::~AppCapturerWin() {
			if (dwmapi_library_)
				FreeLibrary(dwmapi_library_);

			//if(bmpCapture_) DeleteBitmap(bmpCapture_);
			//if(memDcCapture_) DeleteDC(memDcCapture_);
		}		

		bool AppCapturerWin::IsAeroEnabled() {
			BOOL result = FALSE;
			if (is_composition_enabled_func_)
				is_composition_enabled_func_(&result);
			return result != FALSE;
		}
		// AppCapturer interface.
		bool AppCapturerWin::GetAppList(AppList* apps){
			return  true;
		}
		bool AppCapturerWin::SelectApp(ProcessId processId) {
			processId_ = processId;
			return true;
		}
		bool AppCapturerWin::BringAppToFront() {
			return true;
		}

		// DesktopCapturer interface.
		void AppCapturerWin::Start(Callback* callback) {
			assert(!callback_);
			assert(callback);

			callback_ = callback;
		}

		void AppCapturerWin::Capture(const DesktopRegion& region) {
			CaptureByWebRTC(region);
		}
		
		BOOL CALLBACK AppCapturerWin::EnumWindowsProc(HWND handle, LPARAM lParam)
		{
			EnumWindowsLPARAM * pEnumWindowsLPARAM = (EnumWindowsLPARAM *)lParam;
			if(pEnumWindowsLPARAM==NULL) return FALSE;

			DWORD procId = -1;
			GetWindowThreadProcessId( handle, &procId );
			if(procId==pEnumWindowsLPARAM->process_id)
			{
				WindowItem window_item;
				window_item.hWnd = handle;

				if(!::IsWindowVisible(handle)
					|| ::IsIconic(handle)) 
					return TRUE;

				GetWindowRect(handle, &window_item.rcWindow);
				window_item.bSizeChanged = true;
				pEnumWindowsLPARAM->vecWindows.push_back(window_item);
			}
			return TRUE;
		}
		//Application Capturer by refer WebRTC Window Capture
		void AppCapturerWin::CaptureByWebRTC(const DesktopRegion& region) {

			//List Windows of selected application
			EnumWindowsLPARAM lParamEnumWindows;
			lParamEnumWindows.process_id = processId_;
			::EnumWindows(EnumWindowsProc,(LPARAM)&lParamEnumWindows);

			for(std::vector<WindowItem>::reverse_iterator itItem=lParamEnumWindows.vecWindows.rbegin();itItem!=lParamEnumWindows.vecWindows.rend();itItem++)
			{
				for(std::vector<WindowItem>::reverse_iterator itItemPrevious=vec_previous_windows_.rbegin();itItemPrevious!=vec_previous_windows_.rend();itItemPrevious++)
				{
					WindowItem window_item = *itItem;
					WindowItem window_item_prev = *itItemPrevious;
					if(window_item_prev.hWnd == window_item.hWnd 
						&& (window_item.rcWindow.right - window_item.rcWindow.left) == (window_item_prev.rcWindow.right - window_item_prev.rcWindow.left) 
						&& (window_item.rcWindow.bottom - window_item.rcWindow.top) == (window_item_prev.rcWindow.bottom - window_item_prev.rcWindow.top)  )
						itItem->bSizeChanged = false;
				}
			}

			//Prepare capture dc context
			DesktopRect rcDesktop_(DesktopRect::MakeXYWH(
				GetSystemMetrics(SM_XVIRTUALSCREEN),
				GetSystemMetrics(SM_YVIRTUALSCREEN),
				GetSystemMetrics(SM_CXVIRTUALSCREEN),
				GetSystemMetrics(SM_CYVIRTUALSCREEN)));

			HDC dcScreen  = GetDC(NULL);
			HDC memDcCapture_ = CreateCompatibleDC(dcScreen);
			if(dcScreen) ReleaseDC(NULL,dcScreen);

			scoped_ptr<DesktopFrameWin> frameCapture(DesktopFrameWin::Create(
				DesktopSize(rcDesktop_.width(),rcDesktop_.height()),
				NULL, memDcCapture_));
			HBITMAP bmpOrigin = (HBITMAP)::SelectObject(memDcCapture_,frameCapture->bitmap());

			BOOL bCaptureAppResult =  false;
			//Capture and Combine all windows into memDcCapture_
			for(std::vector<WindowItem>::reverse_iterator itItem=lParamEnumWindows.vecWindows.rbegin();itItem!=lParamEnumWindows.vecWindows.rend();itItem++)
			{
				WindowItem window_item = *itItem;
				HWND hWndCapturer = window_item.hWnd;
				if(!::IsWindow(hWndCapturer) 
					|| !::IsWindowVisible(hWndCapturer)
					|| ::IsIconic(hWndCapturer)) continue;

				//
				HDC memDcWin = NULL;
				HBITMAP bmpOriginWin = NULL;
				HDC dcWin =  NULL;
				RECT rcWin= window_item.rcWindow ;
				bool bCaptureResult = false;
				scoped_ptr<DesktopFrameWin> frame;
				do{
					if (rcWin.left == rcWin.right || rcWin.top == rcWin.bottom) {
						break;
					}

					dcWin = GetWindowDC(hWndCapturer);
					if (!dcWin) {
						break;
					} 

					frame.reset(DesktopFrameWin::Create(
						DesktopSize(rcWin.right - rcWin.left, rcWin.bottom - rcWin.top),
						NULL, dcWin));
					if (!frame.get()) {
						break;
					}
					memDcWin = CreateCompatibleDC(dcWin);
					bmpOriginWin = (HBITMAP)SelectObject(memDcWin, frame->bitmap());

					if (!IsAeroEnabled() || window_item.bSizeChanged ) {
						bCaptureResult = PrintWindow(hWndCapturer, memDcWin, 0);
					}

					// Aero is enabled or PrintWindow() failed, use BitBlt.
					if (!bCaptureResult) {
						bCaptureResult = BitBlt(memDcWin, 0, 0, frame->size().width(), frame->size().height(),
							dcWin, 0, 0, SRCCOPY);
					}
				}while(0);

				//bitblt to capture memDcCapture_
				if(bCaptureResult){
					BitBlt(memDcCapture_, 
						rcWin.left, rcWin.top, rcWin.right - rcWin.left,rcWin.bottom - rcWin.top, 
						memDcWin, 0, 0, SRCCOPY);
					bCaptureAppResult = true;
				}

				//Clean resource 
				if(memDcWin){
					SelectObject(memDcWin, bmpOrigin);
					DeleteDC(memDcWin);
				}
				if(dcWin)
					ReleaseDC(hWndCapturer, dcWin);

			}
			vec_previous_windows_ = lParamEnumWindows.vecWindows;

			//Clean resource
			if(memDcCapture_){
				SelectObject(memDcCapture_, bmpOrigin);
				DeleteDC(memDcCapture_);
			}
			
			//trigger event 
			if(bCaptureAppResult)
				callback_->OnCaptureCompleted(frameCapture.release());
		}

		//Application Capturer by sample and region
		void AppCapturerWin::CaptureBySample(const DesktopRegion& region){

		}

		//Application Capturer by system magnifier
		void AppCapturerWin::CaptureByMagnifier(const DesktopRegion& region){

		}

	}  // namespace

	// static
	AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
		return new AppCapturerWin();
	}

}  // namespace webrtc
