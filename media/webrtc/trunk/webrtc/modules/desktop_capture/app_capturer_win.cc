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
#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_frame_win.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

#include <vector>

namespace webrtc {
    
	namespace {
		
		class WindowsCapturerProxy : DesktopCapturer::Callback {
		public:
			WindowsCapturerProxy():window_capturer_(WindowCapturer::Create()){
				window_capturer_->Start(this);
			}
			~WindowsCapturerProxy(){}
			void SelectWindow(HWND hwnd)  { window_capturer_->SelectWindow(reinterpret_cast<WindowId>(hwnd)); }
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
        
		class AppCapturerWin : public AppCapturer {
		public:
			AppCapturerWin();
			virtual ~AppCapturerWin();
            
			// AppCapturer interface.
			virtual bool GetAppList(AppList* apps) OVERRIDE;
			virtual bool SelectApp(ProcessId processId) OVERRIDE;
			virtual bool BringAppToFront() OVERRIDE;
            
			// DesktopCapturer interface.
			virtual void Start(Callback* callback) OVERRIDE;
			virtual void Capture(const DesktopRegion& region) OVERRIDE;
            
			struct WindowItem{
				HWND hWnd;
				RECT rcWindow;
				bool bIsBelongSharedProcess;
			};
			struct EnumWindowsLPARAM {
				ProcessId process_id;
				std::vector<WindowItem> vecWindows;
				bool bListAll;
			};
			static BOOL CALLBACK EnumWindowsProc(HWND handle, LPARAM lParam);
		protected:
			void CaptureByWebRTC(const DesktopRegion& region);
			void CaptureBySample(const DesktopRegion& region);
			void CaptureByMagnifier(const DesktopRegion& region);
		private:
			Callback* callback_;
			ProcessId processId_;
			
			//Sample Mode
			ScreenCapturerProxy screen_capturer_proxy_;
			void updateRegions();
			void printRgnBox(HRGN hrgn);
			HRGN hrgn_foreground_;
			HRGN hrgn_background_;
			HRGN hrgn_visual_;
            
			//WebRtc Window mode
			WindowsCapturerProxy window_capturer_proxy_;
            
			DISALLOW_COPY_AND_ASSIGN(AppCapturerWin);
		};
		//=========================================
		//
		AppCapturerWin::AppCapturerWin()
        : callback_(NULL),
        processId_(NULL){
            
			//Sample mode
			hrgn_foreground_ = CreateRectRgn(0,0,0,0);
			hrgn_background_ = CreateRectRgn(0,0,0,0);
			hrgn_visual_ = CreateRectRgn(0,0,0,0);
		}
        
		AppCapturerWin::~AppCapturerWin() {
            
			//Sample mode
			if(hrgn_foreground_) DeleteObject(hrgn_foreground_);
			if(hrgn_background_) DeleteObject(hrgn_background_);
			if(hrgn_visual_) DeleteObject(hrgn_visual_);
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
			//CaptureByWebRTC(region);
			CaptureBySample(region);
		}
		
		BOOL CALLBACK AppCapturerWin::EnumWindowsProc(HWND handle, LPARAM lParam)
		{
			EnumWindowsLPARAM * pEnumWindowsLPARAM = (EnumWindowsLPARAM *)lParam;
			if(pEnumWindowsLPARAM==NULL) return FALSE;
            
			DWORD procId = -1;
			GetWindowThreadProcessId( handle, &procId );
			if(procId==pEnumWindowsLPARAM->process_id
               || pEnumWindowsLPARAM->bListAll)
			{
				WindowItem window_item;
				window_item.hWnd = handle;
                
				if(!::IsWindowVisible(handle)
                   || ::IsIconic(handle))
					return TRUE;
                
				GetWindowRect(handle, &window_item.rcWindow);
				window_item.bIsBelongSharedProcess = (procId==pEnumWindowsLPARAM->process_id);
				pEnumWindowsLPARAM->vecWindows.push_back(window_item);
			}
			return TRUE;
		}
        
		void AppCapturerWin::CaptureByWebRTC(const DesktopRegion& region) {
            
			//List Windows of selected application
			EnumWindowsLPARAM lParamEnumWindows;
			lParamEnumWindows.process_id = processId_;
			lParamEnumWindows.bListAll = false;
			::EnumWindows(EnumWindowsProc,(LPARAM)&lParamEnumWindows);
            
			//Prepare capture dc context
			DesktopRect rcDesktop(DesktopRect::MakeXYWH(
                                                        GetSystemMetrics(SM_XVIRTUALSCREEN),
                                                        GetSystemMetrics(SM_YVIRTUALSCREEN),
                                                        GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                                        GetSystemMetrics(SM_CYVIRTUALSCREEN)));
            
			HDC dcScreen  = GetDC(NULL);
			HDC memDcCapture = CreateCompatibleDC(dcScreen);
			if(dcScreen) ReleaseDC(NULL,dcScreen);
            
			scoped_ptr<DesktopFrameWin> frameCapture(DesktopFrameWin::Create(
                                                                             DesktopSize(rcDesktop.width(),rcDesktop.height()),
                                                                             NULL, memDcCapture));
			HBITMAP bmpOrigin = (HBITMAP)::SelectObject(memDcCapture,frameCapture->bitmap());
			BOOL bCaptureAppResult =  false;
			//Capture and Combine all windows into memDcCapture
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
				HBITMAP hBitmapFrame = NULL;
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
					memDcWin = CreateCompatibleDC(dcWin);
					//Capture
					window_capturer_proxy_.SelectWindow(hWndCapturer);
					window_capturer_proxy_.Capture(region);
					if(window_capturer_proxy_.GetFrame()!=NULL){
						DesktopFrameWin * pDesktopFrameWin =reinterpret_cast<DesktopFrameWin *>(window_capturer_proxy_.GetFrame().get());
						if(pDesktopFrameWin)
							hBitmapFrame = pDesktopFrameWin->bitmap();
						if(GetObjectType(hBitmapFrame)!=OBJ_BITMAP)
							hBitmapFrame = NULL;
					}
					if(hBitmapFrame==NULL)
						break;
					bmpOriginWin = (HBITMAP)SelectObject(memDcWin,hBitmapFrame);
                    
				}while(0);
                
				//bitblt to capture memDcCapture
				if(bmpOriginWin){
					BitBlt(memDcCapture,
                           rcWin.left, rcWin.top, rcWin.right - rcWin.left,rcWin.bottom - rcWin.top,
                           memDcWin, 0, 0, SRCCOPY);
					bCaptureAppResult = true;
				}
                
				//Clean resource
				if(memDcWin){
					SelectObject(memDcWin, bmpOriginWin);
					DeleteDC(memDcWin);
				}
				if(dcWin)
					ReleaseDC(hWndCapturer, dcWin);
			}
			//Clean resource
			if(memDcCapture){
				SelectObject(memDcCapture, bmpOrigin);
				DeleteDC(memDcCapture);
			}
			
			//trigger event
			if(bCaptureAppResult)
				callback_->OnCaptureCompleted(frameCapture.release());
		}
        
		//Application Capturer by sample and region
		void AppCapturerWin::CaptureBySample(const DesktopRegion& region){
            
			//capture entire screen
			screen_capturer_proxy_.Capture(region);
			
			HBITMAP hBitmapFrame = NULL;
			if(screen_capturer_proxy_.GetFrame()!=NULL){
				SharedDesktopFrame* pSharedDesktopFrame = reinterpret_cast<SharedDesktopFrame*>(screen_capturer_proxy_.GetFrame().get());
				if(pSharedDesktopFrame){
					DesktopFrameWin * pDesktopFrameWin =reinterpret_cast<DesktopFrameWin *>(pSharedDesktopFrame->GetUnderlyingFrame());
					if(pDesktopFrameWin)
						hBitmapFrame = pDesktopFrameWin->bitmap();
					if(GetObjectType(hBitmapFrame)!=OBJ_BITMAP)
						hBitmapFrame = NULL;
				}
			}
			if(hBitmapFrame){
				//calculate app visual/foreground region
				updateRegions();
                
				HDC dcScreen  = GetDC(NULL);
				HDC memDcCapture = CreateCompatibleDC(dcScreen);
				
				RECT rcScreen= {0,0,
					screen_capturer_proxy_.GetFrame()->size().width(),
					screen_capturer_proxy_.GetFrame()->size().height()};
                
				HBITMAP bmpOriginCapture = (HBITMAP)::SelectObject(memDcCapture,hBitmapFrame);
				
				//fill background
				SelectClipRgn(memDcCapture,hrgn_background_);
				SelectObject(memDcCapture, GetStockObject(DC_BRUSH));
				SetDCBrushColor(memDcCapture,RGB(0,0,0));
				FillRect(memDcCapture,&rcScreen,(HBRUSH)GetStockObject(DC_BRUSH));
                
				//fill visible
				/*SelectClipRgn(memDcCapture,hrgn_visual_);
                 BitBlt(memDcScreen,
                 rcScreen.left,
                 rcScreen.top,
                 rcScreen.right-rcScreen.left,
                 rcScreen.bottom-rcScreen.top,memDcScreen,
                 rcScreen.left,
                 rcScreen.top,SRCCOPY);*/
                
				//fill foreground
				SelectClipRgn(memDcCapture,hrgn_foreground_);
				SelectObject(memDcCapture, GetStockObject(DC_BRUSH));
				SetDCBrushColor(memDcCapture,RGB(0xff,0xff,0));
				FillRect(memDcCapture,&rcScreen,(HBRUSH)GetStockObject(DC_BRUSH));
                
				if(dcScreen) ReleaseDC(NULL,dcScreen);
				SelectObject(memDcCapture,bmpOriginCapture);
				DeleteDC(memDcCapture);
                
				//trigger event
				if(callback_)
					callback_->OnCaptureCompleted(screen_capturer_proxy_.GetFrame().release());
				return ;
			}
            
			//trigger event
			if(callback_)
				callback_->OnCaptureCompleted(screen_capturer_proxy_.GetFrame().release());
		}
        
		void AppCapturerWin::printRgnBox(HRGN hrgn){
			RECT rcRgnBox={0,0,0,0};
			int nRet = GetRgnBox(hrgn,&rcRgnBox);
			switch (nRet)
			{
                case NULLREGION:
                    LOG(LS_INFO) << "AppCapturerWin::printRgnBox NULLREGION ,"
					<< "left=" << rcRgnBox.left
					<< "right="<<rcRgnBox.right
					<< "top="<<rcRgnBox.top
					<< "bottom="<<rcRgnBox.bottom;
                    break;
                    
                case SIMPLEREGION:
                    LOG(LS_INFO) << "AppCapturerWin::printRgnBox SIMPLEREGION ,"
					<< "left=" << rcRgnBox.left
					<< "right="<<rcRgnBox.right
					<< "top="<<rcRgnBox.top
					<< "bottom="<<rcRgnBox.bottom;
                    break;
                    
                case COMPLEXREGION:
                    LOG(LS_INFO) << "AppCapturerWin::printRgnBox COMPLEXREGION ,"
					<< "left=" << rcRgnBox.left
					<< "right="<<rcRgnBox.right
					<< "top="<<rcRgnBox.top
					<< "bottom="<<rcRgnBox.bottom;
                    break;
                    
                default:
                    LOG(LS_ERROR) << "AppCapturerWin::printRgnBox" << GetLastError();
                    break;
			}
		}
		void AppCapturerWin::updateRegions(){
			//List Windows of selected application
			EnumWindowsLPARAM lParamEnumWindows;
			lParamEnumWindows.process_id = processId_;
			lParamEnumWindows.bListAll = true;
			::EnumWindows(EnumWindowsProc,(LPARAM)&lParamEnumWindows);
            
			SetRectRgn(hrgn_foreground_,0,0,0,0);
			SetRectRgn(hrgn_visual_,0,0,0,0);
			SetRectRgn(hrgn_background_,0,0,0,0);
			
			HRGN hrgn_screen_ = CreateRectRgn(0,
                                              0,
                                              GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                              GetSystemMetrics(SM_CYVIRTUALSCREEN));
            
			HRGN hrgn_window = CreateRectRgn(0,0,0,0);
			HRGN hrgn_internsect = CreateRectRgn(0,0,0,0);
			for(std::vector<WindowItem>::reverse_iterator itItem=lParamEnumWindows.vecWindows.rbegin();itItem!=lParamEnumWindows.vecWindows.rend();itItem++){
				WindowItem window_item = *itItem;
				SetRectRgn(hrgn_window,0,0,0,0);
				if(GetWindowRgn(window_item.hWnd,hrgn_window)==ERROR){
					SetRectRgn(hrgn_window,window_item.rcWindow.left,
                               window_item.rcWindow.top,
                               window_item.rcWindow.right,
                               window_item.rcWindow.bottom);
				}
				printRgnBox(hrgn_window);
                
				if(window_item.bIsBelongSharedProcess){
					CombineRgn(hrgn_visual_,hrgn_visual_,hrgn_window,RGN_OR);
					CombineRgn(hrgn_foreground_,hrgn_foreground_,hrgn_window,RGN_DIFF);
                    
					printRgnBox(hrgn_visual_);
					printRgnBox(hrgn_foreground_);
				}
				else{
					SetRectRgn(hrgn_internsect,0,0,0,0);
					CombineRgn(hrgn_internsect,hrgn_visual_,hrgn_window,RGN_AND);
					printRgnBox(hrgn_internsect);
                    
					CombineRgn(hrgn_visual_,hrgn_visual_,hrgn_internsect,RGN_DIFF);
                    
					CombineRgn(hrgn_foreground_,hrgn_foreground_,hrgn_internsect,RGN_OR);
                    
					printRgnBox(hrgn_visual_);
					printRgnBox(hrgn_foreground_);
				}
			}
            
			//
			CombineRgn(hrgn_background_,hrgn_screen_,hrgn_visual_,RGN_DIFF);
            
			printRgnBox(hrgn_foreground_);
			printRgnBox(hrgn_visual_);
			printRgnBox(hrgn_background_);
            
			if(hrgn_window) DeleteObject(hrgn_window);
			if(hrgn_internsect) DeleteObject(hrgn_internsect);
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
