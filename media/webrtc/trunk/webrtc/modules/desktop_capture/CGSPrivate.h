#ifndef _CGS_PRIVIATE_H_
#define _CGS_PRIVIATE_H_
//Declare private CoreGraphics APIs
//==============================================================
typedef enum _CGSWindowOrderingMode {
    kCGSOrderAbove                =  1, // Window is ordered above target.
    kCGSOrderBelow                = -1, // Window is ordered below target.
    kCGSOrderOut                  =  0  // Window is removed from the on-screen window list.
} CGSWindowOrderingMode;

typedef enum {
	CGSTagExposeFade	= 0x0002,   // Fade out when Expose activates.
	CGSTagNoShadow		= 0x0008,   // No window shadow.
	CGSTagTransparent   = 0x0200,   // Transparent to mouse clicks.
	CGSTagSticky		= 0x0800,   // Appears on all workspaces.
} CGSWindowTag;

extern "C"
{
	typedef int CGSConnection;
	typedef int CGSWindow;
	
	extern CGSConnection _CGSDefaultConnection();
	extern CGError CGSGetConnectionIDForPSN(const CGSConnection connection, ProcessSerialNumber * psn, CGSConnection * targetConnection);
	
	extern CGError CGSGetOnScreenWindowCount(const CGSConnection connection, CGSConnection targetConnection, int * count);
	extern CGError CGSGetOnScreenWindowList(const CGSConnection connection, CGSConnection targetConnection, int count, int * list, int * outCount);
    
	extern OSStatus CGSGetWindowCount(const CGSConnection cid, CGSConnection targetCID, int* outCount);
	extern OSStatus CGSGetWindowList(const CGSConnection cid, CGSConnection targetCID,int count, int* list, int* outCount);
    
	extern OSStatus CGSGetScreenRectForWindow(const CGSConnection cid, CGSWindow wid, CGRect *outRect);
	extern OSStatus CGSGetWindowOwner(const CGSConnection cid, const CGSWindow wid, CGSConnection *ownerCid);
	
	extern CGError CGSSetWindowListBrightness(CGSConnection,int*,float*,int);
	
	extern OSStatus CGSGetWindowTags(const CGSConnection cid, const CGSWindow wid, CGSWindowTag *tags, int thirtyTwo);
	extern OSStatus CGSSetWindowTags(const CGSConnection cid, const CGSWindow wid, CGSWindowTag *tags, int thirtyTwo);
    
	extern OSStatus CGSGetWindowOwnerPSN(const CGSConnection cid, const CGSWindow wid, ProcessSerialNumber *ownerPSN);
	
	extern OSStatus CGSGetWindowProperty(const CGSConnection cid, const CGSWindow wid,CFStringRef propKeyRef,CFTypeRef* outValue);
    
	extern OSStatus CGSGetWindowBounds(int,int,CGRect*);
	extern OSStatus CGSGetScreenRectForWindow(const CGSConnection cid, CGSWindow wid, CGRect *outRect);
	
	extern OSStatus CGSConnectionGetPID(const CGSConnection cid, pid_t *pid, const CGSConnection ownerCid);
	
	extern void CGContextCopyDisplayCaptureContentsToRect(CGContextRef,CGRect,int,int,CGRect);
	extern void CGContextCopyWindowCaptureContentsToRect(CGContextRef,CGRect,int,int,CGRect);
	
	extern void CGContextCopyWindowContentsToRect(CGContextRef,CGRect,int,int,CGRect);
	
	extern OSStatus CGSOrderWindow(const CGSConnection cid, const CGSWindow wid,
                                   CGSWindowOrderingMode place, CGSWindow relativeToWindowID /* can be NULL */);
	
	CGError CGSGetSharedWindow(CGSConnection,CGSWindow,int);
	
	int GetNativeWindowFromWindowRef(WindowRef);
	OSStatus CGSGetWindowAlpha(const CGSConnection cid, const CGSWindow wid, float* alpha);
	
	CGError CGSGetWindowType(const CGSConnection cid, CGSWindow inWindow ,UInt32 *outWindowType );
}
#endif //_CGS_PRIVIATE_H_