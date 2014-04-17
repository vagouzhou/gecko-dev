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

namespace webrtc {

	namespace {

		// Convenience wrapper for XGetWindowProperty() results.
		template <class PropertyType>
		class XWindowProperty {
		public:
			XWindowProperty(Display* display, Window window, Atom property)
				: is_valid_(false),
				size_(0),
				data_(NULL) {
					const int kBitsPerByte = 8;
					Atom actual_type;
					int actual_format;
					unsigned long bytes_after;  // NOLINT: type required by XGetWindowProperty
					int status = XGetWindowProperty(display, window, property, 0L, ~0L, False,
						AnyPropertyType, &actual_type,
						&actual_format, &size_,
						&bytes_after, &data_);
					if (status != Success) {
						data_ = NULL;
						return;
					}
					if (sizeof(PropertyType) * kBitsPerByte != actual_format) {
						size_ = 0;
						return;
					}

					is_valid_ = true;
			}

			~XWindowProperty() {
				if (data_)
					XFree(data_);
			}

			// True if we got properly value successfully.
			bool is_valid() const { return is_valid_; }

			// Size and value of the property.
			size_t size() const { return size_; }
			const PropertyType* data() const {
				return reinterpret_cast<PropertyType*>(data_);
			}
			PropertyType* data() {
				return reinterpret_cast<PropertyType*>(data_);
			}

		private:
			bool is_valid_;
			unsigned long size_;  // NOLINT: type required by XGetWindowProperty
			unsigned char* data_;

			DISALLOW_COPY_AND_ASSIGN(XWindowProperty);
		};

		class AppCapturerLinux : public AppCapturer,
			public SharedXDisplay::XEventHandler {
		public:
			AppCapturerLinux(const DesktopCaptureOptions& options);
			virtual ~AppCapturerLinux();

			// AppCapturer interface.
			virtual bool GetWindowList(WindowList* windows) OVERRIDE;
			virtual bool SelectWindow(ProcessId id) OVERRIDE;
			virtual bool BringSelectedWindowToFront() OVERRIDE;

			// DesktopCapturer interface.
			virtual void Start(Callback* callback) OVERRIDE;
			virtual void Capture(const DesktopRegion& region) OVERRIDE;

			// SharedXDisplay::XEventHandler interface.
			virtual bool HandleXEvent(const XEvent& event) OVERRIDE;

		private:
			Display* display() { return x_display_->display(); }

			// Iterates through |window| hierarchy to find first visible window, i.e. one
			// that has WM_STATE property set to NormalState.
			// See http://tronche.com/gui/x/icccm/sec-4.html#s-4.1.3.1 .
			::Window GetApplicationWindow(::Window window);

			// Returns true if the |window| is a desktop element.
			bool IsDesktopElement(::Window window);

			// Returns window title for the specified X |window|.
			bool GetWindowTitle(::Window window, std::string* title);

			Callback* callback_;

			scoped_refptr<SharedXDisplay> x_display_;

			Atom wm_state_atom_;
			Atom window_type_atom_;
			Atom normal_window_type_atom_;
			bool has_composite_extension_;

			::Window selected_process_;
			XServerPixelBuffer x_server_pixel_buffer_;

			DISALLOW_COPY_AND_ASSIGN(AppCapturerLinux);
		};

		AppCapturerLinux::AppCapturerLinux(const DesktopCaptureOptions& options)
			: callback_(NULL),
			x_display_(options.x_display()),
			has_composite_extension_(false),
			selected_process_(0) {
		}

		AppCapturerLinux::~AppCapturerLinux() {
		}

		// AppCapturer interface.
		virtual bool GetWindowList(WindowList* windows){
			return true;
		}
		virtual bool SelectWindow(ProcessId id){
			return true;
		}
		virtual bool BringSelectedWindowToFront(){
			return true;
		}

		// DesktopCapturer interface.
		void AppCapturerLinux::Start(Callback* callback) {
			assert(!callback_);
			assert(callback);

			callback_ = callback;
		}
		void AppCapturerLinux::Capture(const DesktopRegion& region) {
			//callback_->OnCaptureCompleted(frame);
		}

	}  // namespace

	// static
	AppCapturer* AppCapturer::Create(const DesktopCaptureOptions& options) {
		if (!options.x_display())
			return NULL;
		return new AppCapturerLinux(options);
	}

}  // namespace webrtc
