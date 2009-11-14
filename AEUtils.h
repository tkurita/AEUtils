#include <ApplicationServices/ApplicationServices.h>

CFMutableArrayRef CFMutableArrayCreatePOSIXPathsWithEvent(const AppleEvent *ev, 
														  AEKeyword theKey, OSErr *errPtr);

CFStringRef CFStringCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr);

OSErr getBoolValue(const AppleEvent *ev, AEKeyword theKey,  Boolean *outValue);
OSErr getPOSIXPathArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray);// deprecated
OSErr getFloatArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray);
OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr);// deprecated
OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef);
OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng);

void showAEDesc(const AppleEvent *ev);
void safeRelease(CFTypeRef theObj);
OSErr putStringListToEvent(AppleEvent *ev, AEKeyword keyword, CFArrayRef array, CFStringEncoding kEncoding);
OSErr putBoolToReply(Boolean aBool, AppleEvent *reply);
OSErr putStringToEvent(AppleEvent *ev, AEKeyword keyword, CFStringRef inStr, CFStringEncoding kEncoding);
OSErr putMissingValueToReply(AppleEvent *reply);
OSErr putFilePathToReply(CFURLRef inURL, AppleEvent *reply);
OSErr putAliasToReply(AliasHandle inAlias, AppleEvent *reply);

//deprecated
// use putStringToEvent(AppleEvent *ev, keyAEResult, CFStringRef inStr, CFStringEncoding kEncoding); 
OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply);