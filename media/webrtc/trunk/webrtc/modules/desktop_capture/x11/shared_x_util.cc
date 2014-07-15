#include "webrtc/modules/desktop_capture/x11/shared_x_util.h"

namespace webrtc{


WindowUtilX11::WindowUtilX11(scoped_refptr<SharedXDisplay> x_display){
	x_display_ = x_display;
	wm_state_atom_ = XInternAtom(display(), "WM_STATE", True);
	window_type_atom_ = XInternAtom(display(), "_NET_WM_WINDOW_TYPE", True);
	normal_window_type_atom_ = XInternAtom(display(), "_NET_WM_WINDOW_TYPE_NORMAL", True);
	process_atom_ = XInternAtom(display(), "_NET_WM_PID", True);
}
WindowUtilX11::~WindowUtilX11(){

}
::Window WindowUtilX11::GetApplicationWindow(::Window window){
	// Get WM_STATE property of the window.
	  XWindowProperty<uint32_t> window_state(display(), window, wm_state_atom_);

	  // WM_STATE is considered to be set to WithdrawnState when it missing.
	  int32_t state = window_state.is_valid() ?
	      *window_state.data() : WithdrawnState;

	  if (state == NormalState) {
	    // Window has WM_STATE==NormalState. Return it.
	    return window;
	  } else if (state == IconicState) {
	    // Window is in minimized. Skip it.
	    return 0;
	  }

	  // If the window is in WithdrawnState then look at all of its children.
	  ::Window root, parent;
	  ::Window *children;
	  unsigned int num_children;
	  if (!XQueryTree(display(), window, &root, &parent, &children,
	                  &num_children)) {
	    LOG(LS_ERROR) << "Failed to query for child windows although window"
	                  << "does not have a valid WM_STATE.";
	    return 0;
	  }
	  ::Window app_window = 0;
	  for (unsigned int i = 0; i < num_children; ++i) {
	    app_window = GetApplicationWindow(children[i]);
	    if (app_window)
	      break;
	  }

	  if (children)
	    XFree(children);
	  return app_window;
}
bool WindowUtilX11::IsDesktopElement(::Window window){
	if (window == 0)
	    return false;

	  // First look for _NET_WM_WINDOW_TYPE. The standard
	  // (http://standards.freedesktop.org/wm-spec/latest/ar01s05.html#id2760306)
	  // says this hint *should* be present on all windows, and we use the existence
	  // of _NET_WM_WINDOW_TYPE_NORMAL in the property to indicate a window is not
	  // a desktop element (that is, only "normal" windows should be shareable).
	  XWindowProperty<uint32_t> window_type(display(), window, window_type_atom_);
	  if (window_type.is_valid() && window_type.size() > 0) {
	    uint32_t* end = window_type.data() + window_type.size();
	    bool is_normal = (end != std::find(
	        window_type.data(), end, normal_window_type_atom_));
	    return !is_normal;
	  }

	  // Fall back on using the hint.
	  XClassHint class_hint;
	  Status status = XGetClassHint(display(), window, &class_hint);
	  bool result = false;
	  if (status == 0) {
	    // No hints, assume this is a normal application window.
	    return result;
	  }

	  if (strcmp("gnome-panel", class_hint.res_name) == 0 ||
	      strcmp("desktop_window", class_hint.res_name) == 0) {
	    result = true;
	  }
	  XFree(class_hint.res_name);
	  XFree(class_hint.res_class);
	  return result;
}
bool WindowUtilX11::GetWindowTitle(::Window window, std::string* title){
	int status;
	  bool result = false;
	  XTextProperty window_name;
	  window_name.value = NULL;
	  if (window) {
	    status = XGetWMName(display(), window, &window_name);
	    if (status && window_name.value && window_name.nitems) {
	      int cnt;
	      char **list = NULL;
	      status = Xutf8TextPropertyToTextList(display(), &window_name, &list,
	                                           &cnt);
	      if (status >= Success && cnt && *list) {
	        if (cnt > 1) {
	          LOG(LS_INFO) << "Window has " << cnt
	                       << " text properties, only using the first one.";
	        }
	        *title = *list;
	        result = true;
	      }
	      if (list)
	        XFreeStringList(list);
	    }
	    if (window_name.value)
	      XFree(window_name.value);
	  }
	  return result;
}

bool WindowUtilX11::BringWindowToFront(::Window window){
	if (!window)
	    return false;

	  unsigned int num_children;
	  ::Window* children;
	  ::Window parent;
	  ::Window root;
	  // Find the root window to pass event to.
	  int status = XQueryTree(
	      display(), window, &root, &parent, &children, &num_children);
	  if (status == 0) {
	    LOG(LS_ERROR) << "Failed to query for the root window.";
	    return false;
	  }

	  if (children)
	    XFree(children);

	  XRaiseWindow(display(), window);

	  // Some window managers (e.g., metacity in GNOME) consider it illegal to
	  // raise a window without also giving it input focus with
	  // _NET_ACTIVE_WINDOW, so XRaiseWindow() on its own isn't enough.
	  Atom atom = XInternAtom(display(), "_NET_ACTIVE_WINDOW", True);
	  if (atom != None) {
	    XEvent xev;
	    xev.xclient.type = ClientMessage;
	    xev.xclient.serial = 0;
	    xev.xclient.send_event = True;
	    xev.xclient.window = window;
	    xev.xclient.message_type = atom;

	    // The format member is set to 8, 16, or 32 and specifies whether the
	    // data should be viewed as a list of bytes, shorts, or longs.
	    xev.xclient.format = 32;

	    memset(xev.xclient.data.l, 0, sizeof(xev.xclient.data.l));

	    XSendEvent(display(),
	               root,
	               False,
	               SubstructureRedirectMask | SubstructureNotifyMask,
	               &xev);
	  }
	  XFlush(display());
	  return true;
}

int WindowUtilX11::GetWindowProcessID(::Window window){
	// Get _NET_WM_PID property of the window.
	  XWindowProperty<uint32_t> process_id(display(), window, process_atom_);

	  // WM_STATE is considered to be set to WithdrawnState when it missing.
	  return process_id.is_valid() ? *process_id.data() : 0;
}

}//namespace webrtc
