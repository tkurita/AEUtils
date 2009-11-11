#include <ApplicationServices/ApplicationServices.h>

OSErr getFloatArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray);
OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr);
OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef);
OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng);

void showAEDesc(const AppleEvent *ev);
void safeRelease(CFTypeRef theObj);
OSErr putBoolToReply(Boolean aBool, AppleEvent *reply);
OSErr putStringToEvent(AppleEvent *ev, AEKeyword keyword, CFStringRef inStr, CFStringEncoding kEncoding);
OSErr putMissingValueToReply(AppleEvent *reply);
OSErr putFilePathToReply(CFURLRef inURL, AppleEvent *reply);
OSErr putAliasToReply(AliasHandle inAlias, AppleEvent *reply);

//deprecated
// use putStringToEvent(AppleEvent *ev, keyAEResult, CFStringRef inStr, CFStringEncoding kEncoding); 
OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply);