#include <ApplicationServices/ApplicationServices.h>

#define useDeprected 0
CFMutableArrayRef CFMutableArrayCreatePOSIXPathsWithEvent(const AppleEvent *ev, 
														  AEKeyword theKey, OSErr *errPtr);

CFStringRef CFStringCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr);
CFStringRef CFStringCreateWithAEDesc(const AEDesc *desc, OSErr *errPtr);

CFURLRef CFURLCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr);

OSErr AEDescCreateWithCFString(CFStringRef string, CFStringEncoding kEncoding, AEDesc* outDescPtr);
OSErr AEDescCreateMissingValue(AEDesc *outDescPtr);
OSErr AEDescCreateWithCFURL(CFURLRef url, AEDesc* outDescPtr);
OSErr getBoolValue(const AppleEvent *ev, AEKeyword theKey,  Boolean *outValue);
OSErr getFloatArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray);

OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng);

void safeRelease(CFTypeRef theObj);
OSErr putStringListToEvent(AppleEvent *ev, AEKeyword keyword, CFArrayRef array, CFStringEncoding kEncoding);
OSErr putBooleanToEvent(AppleEvent *ev, AEKeyword keyword, Boolean inBool);
OSErr putFileURLToEvent(AppleEvent *ev, AEKeyword keyword, CFURLRef inURL);

/*
 == keyword 
	result : keyAEResult, error message : keyErrorString 
 == kEncoding
	kCFStringEncodingUTF8, kCFStringEncodingUnicode
*/
OSErr putStringToEvent(AppleEvent *ev, AEKeyword keyword, CFStringRef inStr, CFStringEncoding kEncoding);
OSErr putMissingValueToReply(AppleEvent *reply);
OSErr putFilePathToReply(CFURLRef inURL, AppleEvent *reply);

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
OSErr sourceStringOfAEDesc(ComponentInstance component, AEDesc* inDesc, AEDesc *outDesc);
#endif

#if useDeprected //functions using deprecated API
void showAEDesc(const AppleEvent *ev);
OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef);
OSErr putAliasToReply(AliasHandle inAlias, AppleEvent *reply);
#endif


//deprecated
// 
OSErr putBoolToReply(Boolean aBool, AppleEvent *reply);
// OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply); // use putStringToEvent
// OSErr getPOSIXPathArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray); // use CFMutableArrayCreatePOSIXPathsWithEvent
