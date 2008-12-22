#include <ApplicationServices/ApplicationServices.h>

OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr);
OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef);
OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng);

void showAEDesc(const AppleEvent *ev);
void safeRelease(CFTypeRef theObj);
OSErr putBoolToReply(Boolean aBool, AppleEvent *reply);
OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply);