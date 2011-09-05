#include <ApplicationServices/ApplicationServices.h>

CFMutableArrayRef CFMutableArrayCreatePOSIXPathsWithEvent(const AppleEvent *ev, 
														  AEKeyword theKey, OSErr *errPtr);

CFStringRef CFStringCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr);
CFStringRef CFStringCreateWithAEDesc(const AEDesc *desc, OSErr *errPtr);

CFURLRef CFURLCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr);

OSErr AEDescCreateWithCFString(CFStringRef string, CFStringEncoding kEncoding, AEDesc* outDescPtr);
OSErr AEDescCreateMissingValue(AEDesc *outDescPtr);

OSErr getBoolValue(const AppleEvent *ev, AEKeyword theKey,  Boolean *outValue);
OSErr getFloatArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray);
OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef);
OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng);

void showAEDesc(const AppleEvent *ev);
void safeRelease(CFTypeRef theObj);
OSErr putStringListToEvent(AppleEvent *ev, AEKeyword keyword, CFArrayRef array, CFStringEncoding kEncoding);
OSErr putBoolToReply(Boolean aBool, AppleEvent *reply);

/*
 == keyword 
	result : keyAEResult, error message : keyErrorString 
 == kEncoding
	kCFStringEncodingUTF8, kCFStringEncodingUnicode
*/
OSErr putStringToEvent(AppleEvent *ev, AEKeyword keyword, CFStringRef inStr, CFStringEncoding kEncoding);
OSErr putMissingValueToReply(AppleEvent *reply);
OSErr putFilePathToReply(CFURLRef inURL, AppleEvent *reply);
OSErr putAliasToReply(AliasHandle inAlias, AppleEvent *reply);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
OSErr sourceStringOfAEDesc(ComponentInstance component, AEDesc* inDesc, AEDesc *outDesc);
#endif

//deprecated
// 
// OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply); // use putStringToEvent
// OSErr getPOSIXPathArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray); // use CFMutableArrayCreatePOSIXPathsWithEvent
