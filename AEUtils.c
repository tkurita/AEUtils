#include "AEUtils.h"
#include <sys/param.h>
#include <Carbon/Carbon.h>
#define bufferSize MAXPATHLEN+1 	

#define useLog 0

void showAEDesc(const AppleEvent *ev)
{
	Handle result;
	OSStatus resultStatus;
	resultStatus = AEPrintDescToHandle(ev,&result);
	printf("%s\n",*result);
	DisposeHandle(result);
}

void safeRelease(CFTypeRef theObj)
{
	if (theObj != NULL) {
		CFRelease(theObj);
	}
}

OSErr getBoolValue(const AppleEvent *ev, AEKeyword theKey,  Boolean *outValue)
{
	OSErr err;
	DescType typeCode;
	Size dataSize;
	
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	if (typeCode == typeTrue) {
		*outValue = true;
	} else if(typeCode == typeFalse) {
		*outValue = false;
	}
		
bail:
	return err;
}

OSErr getPOSIXPathArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray)
{
	OSErr err;
	DescType typeCode;
	Size dataSize;
	
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	AEDescList  aeList;
	err = AEGetParamDesc(ev, theKey, typeAEList, &aeList);
	if (err != noErr) goto bail;
	
    long        count = 0;
	err = AECountItems(&aeList, &count);
	if (err != noErr) goto bail;
	
	*outArray = CFArrayCreateMutable(NULL, count, &kCFTypeArrayCallBacks);
	
	for(long index = 1; index <= count; index++) {
		void *value_ptr;
		Size data_size;
		err = AEGetNthPtr(&aeList, index, typeFileURL,
						  NULL, NULL, value_ptr,
						  0, &data_size);
		if (err == noErr) {
			value_ptr = malloc(data_size);
			err = AEGetNthPtr(&aeList, index, typeFileURL,
							  NULL, NULL, value_ptr,
							  data_size, NULL);
		}
		if (err != noErr) {
			fprintf(stderr, "Fail to AEGetNthPtr in getPOSIXPathArray\n");
			goto bail;
		}
		CFURLRef file_url = CFURLCreateAbsoluteURLWithBytes(
												  NULL,
												  (const UInt8 *)value_ptr,
												  data_size,
												  kCFStringEncodingUTF8,
												  NULL,
												  false);
		CFStringRef path = CFURLCopyFileSystemPath(file_url, kCFURLPOSIXPathStyle);
		CFArrayAppendValue(*outArray, path);		
		CFRelease(file_url);
		CFRelease(path);
		free(value_ptr);
    }
bail:
#if useLog
	CFShow(*outArray);
	fprintf(stderr, "end of getPOSIXPathArray\n");
#endif	
	return err;
}

OSErr getURLFromUTextDesc(const AEDesc *utdesc_p, CFURLRef *urlRef_p)
{
	OSErr err;
	Size theLength = AEGetDescDataSize(utdesc_p);
	
	UInt8 *theData = malloc(theLength);
	err = AEGetDescData(utdesc_p, theData, theLength);
	if (err != noErr) goto bail;
	
	CFStringRef pathStr = CFStringCreateWithBytes(NULL, theData, theLength, kCFStringEncodingUnicode, false);
	
	CFURLPathStyle pathStyle;
	if (CFStringHasPrefix(pathStr, CFSTR("/"))) {
		pathStyle = kCFURLPOSIXPathStyle;
	}
	else {
		pathStyle = kCFURLHFSPathStyle;
	}	
	*urlRef_p = CFURLCreateWithFileSystemPath(NULL, pathStr, pathStyle, true);
	CFRelease(pathStr);
	
bail:
	free(theData);
	return err;
}

OSStatus getFSRefFromUTextAE(const AppleEvent *ev, AEKeyword theKey, FSRef *ref_p)
{
	AEDesc givenDesc;
	OSStatus err = AEGetParamDesc(ev, theKey, typeUnicodeText, &givenDesc);
#if useLog
	showAEDesc(&givenDesc);
#endif
	if (err != noErr) goto bail;
	
	CFURLRef urlRef = NULL;
	err = getURLFromUTextDesc(&givenDesc, &urlRef);
	if (err != noErr) goto bail;
	
	Boolean canGetFSRef = 0;
	if (urlRef != NULL) {
		canGetFSRef = CFURLGetFSRef(urlRef, ref_p);
	}
	
	if (! canGetFSRef) {
		err = errAECoercionFail;
	}

bail:
	safeRelease(urlRef);
	return err;
}

OSStatus getFSRefFromAE(const AppleEvent *ev, AEKeyword theKey, FSRef *ref_p)
{
	AEDesc givenDesc;
	OSStatus err = AEGetParamDesc(ev, keyDirectObject, typeFSRef, &givenDesc);
#if useLog
	showAEDesc(&givenDesc);
#endif
	err = AEGetDescData(&givenDesc, ref_p, sizeof(FSRef));
	return err;
}

OSErr getFSRef(const AppleEvent *ev, AEKeyword theKey, FSRef *outFSRef_p)
{
	OSErr err = noErr;		
	DescType typeCode;
	Size dataSize;
	err = AESizeOfParam(ev, keyDirectObject, &typeCode, &dataSize);
	
	if (err != noErr) goto bail;
	
	switch (typeCode) {
		case typeAlias:
#if !__LP64__			
		case typeFSS:
#endif
		case typeFileURL:
		case cObjectSpecifier:
			err = getFSRefFromAE(ev, theKey, outFSRef_p);
			break;
		case typeChar:
		case typeUTF8Text:
		case typeUnicodeText:
			err = getFSRefFromUTextAE(ev, theKey, outFSRef_p);
			break;
		default:
			err = errAEWrongDataType;
	}

bail:	
	return err;
}

CFNumberType CFNumberTypeWithAENumberType(DescType typeCode)
{
	CFNumberType result = 0;
	switch (typeCode) {
		case typeSInt16:
			result = kCFNumberSInt16Type;
			break;
		case typeSInt32:
			result = kCFNumberSInt32Type;
			break;
		case typeSInt64:
			result = kCFNumberSInt64Type;
			break;
		case typeIEEE32BitFloatingPoint:
			result =  kCFNumberFloat32Type;
			break;
		case typeIEEE64BitFloatingPoint:
			result = kCFNumberFloat64Type;
			break;
		default:
			break;
	}
	return result;
}

OSErr getFloatArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray)
{
	OSErr err;
	DescType typeCode;
	Size dataSize;
	
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	AEDescList  aeList;
	err = AEGetParamDesc(ev, theKey, typeAEList, &aeList);
	if (err != noErr) goto bail;
	
    long        count = 0;
	err = AECountItems(&aeList, &count);
	if (err != noErr) goto bail;
	
	
	*outArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	
    for(long index = 1; index <= count; index++) {
		float value;
		err = AEGetNthPtr(&aeList, index, typeIEEE32BitFloatingPoint,
						  NULL, NULL, &value,
						  sizeof(value), NULL);
		if (err != noErr) {
			fprintf(stderr, "Fail to AEGetNthPtr in getFloatArray\n");
			goto bail;
		}
		CFNumberRef cfnum = CFNumberCreate(NULL, kCFNumberFloat32Type, &value);
		CFArrayAppendValue(*outArray, cfnum);
		CFRelease(cfnum);
    }
bail:
#if useLog
	CFShow(*outArray);
	fprintf(stderr, "end of getFloatArray\n");
#endif	
	return err;
}

OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr)
{
#if useLog
	printf("start getStringValue\n");
#endif
	OSErr err;
	DescType typeCode;
	DescType returnedType;
    Size actualSize;
	Size dataSize;
	CFStringEncoding encodeKey;
	OSType a_type;
	
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	if (dataSize == 0) {
		*outStr = CFSTR("");
		goto bail;
	}
	
	switch (typeCode) {
		case typeChar:
			encodeKey = CFStringGetSystemEncoding();
			break;
		case typeUTF8Text:
			encodeKey = kCFStringEncodingUTF8;
			break;
		case typeType:
			err = AEGetParamPtr (ev, theKey, typeCode, &returnedType, &a_type, dataSize, &actualSize);
			if (a_type == cMissingValue) {
				goto bail;
			}
			//break;
		default :
			typeCode = typeUnicodeText;
			encodeKey = kCFStringEncodingUnicode;
	}
	
	UInt8 *dataPtr = malloc(dataSize);
	err = AEGetParamPtr (ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	if (actualSize > dataSize) {
#if useLog
		printf("buffere size is allocated. data:%i actual:%i\n", dataSize, actualSize);
#endif	
		dataSize = actualSize;
		dataPtr = (UInt8 *)realloc(dataPtr, dataSize);
		if (dataPtr == NULL) {
			printf("fail to reallocate memory\n");
			goto bail;
		}
		err = AEGetParamPtr (ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	}
	
	if (err != noErr) {
		free(dataPtr);
		goto bail;
	}
	
	*outStr = CFStringCreateWithBytes(NULL, dataPtr, dataSize, encodeKey, false);
	free(dataPtr);
bail:
#if useLog		
	printf("end getStringValue\n");
#endif
	return err;
}

OSErr isMissingValue(const AppleEvent *ev, AEKeyword theKey, Boolean *ismsng)
{
	OSErr err;
	DescType typeCode;
	Size dataSize;
	
	*ismsng = false;
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	if (typeCode == cMissingValue) {
		*ismsng = true;
	}
	
bail:
	return noErr;
}

OSErr CFStringToAEDesc(CFStringRef string, CFStringEncoding kEncoding, AEDesc* outDescPtr)
{
	// kEncoding can be omitted to specify with giving NULL
	OSErr err;
	DescType resultType;
	
	switch (kEncoding) {
		case kCFStringEncodingUTF8 :
			resultType = typeUTF8Text;
			break;
		default :
			resultType = typeUnicodeText;
	}
	
	const char *constBuff = CFStringGetCStringPtr(string, kEncoding);
	if (constBuff == NULL) {
		char *buffer;
		CFIndex charLen = CFStringGetLength(string);
		CFIndex maxLen = CFStringGetMaximumSizeForEncoding(charLen, kEncoding)+1; // +1 is for null termination.
		buffer = malloc(sizeof(char)*maxLen);
		CFStringGetCString(string, buffer, maxLen, kEncoding);
		err=AECreateDesc(resultType, buffer, strlen(buffer), outDescPtr);
		free(buffer);
	}
	else {
		err=AECreateDesc(resultType, constBuff, strlen(constBuff), outDescPtr);
	}
	
	return err;
}

OSErr putStringListToEvent(AppleEvent *ev, AEKeyword keyword, CFArrayRef array, CFStringEncoding kEncoding)
{
	OSErr err;
	AEDescList resultList;
	err = AECreateList(NULL, 0, FALSE, &resultList);
	
	for (int n = 0; n < CFArrayGetCount(array); n++) {
		CFStringRef string = CFArrayGetValueAtIndex(array, n);
		AEDesc string_desc;
		err = CFStringToAEDesc(string, kEncoding, &string_desc);
		if (err != noErr) goto bail;
		err = AEPutDesc(&resultList, n+1, &string_desc);
		AEDisposeDesc(&string_desc);
	}
	
	err = AEPutParamDesc(ev, keyword, &resultList);
bail:
	AEDisposeDesc(&resultList);
	return err;
}

OSErr putStringToEvent(AppleEvent *ev, AEKeyword keyword, CFStringRef inStr, CFStringEncoding kEncoding)
{
#if useLog
	fprintf(stderr, "start putStringToEvent\n");
#endif
	OSErr err;	
	AEDesc resultDesc;
	err = CFStringToAEDesc(inStr, kEncoding, &resultDesc);
	if (err != noErr) goto bail;
	
	err = AEPutParamDesc(ev, keyword, &resultDesc);
	/*
	if (err != noErr) {
		AEDisposeDesc(&resultDesc);
	}
	 */
	AEDisposeDesc(&resultDesc);
bail:
#if useLog
	fprintf(stderr, "end putStringToEvent\n");
#endif
	return err;
}

OSErr putStringToReply(CFStringRef inStr, CFStringEncoding kEncoding, AppleEvent *reply)
{
// kEncoding can be omitted to specify with giving NULL
	return putStringToEvent(reply, keyAEResult, inStr, kEncoding);
}

OSErr putBoolToReply(Boolean aBool, AppleEvent *reply)
{
#if useLog
	printf("start putBoolToReply\n");
#endif
	OSErr err;
	DescType resultType = (aBool? typeTrue:typeFalse);
	AEDesc resultDesc;
	err=AECreateDesc(resultType, NULL, 0, &resultDesc);
	err=AEPutParamDesc(reply, keyAEResult, &resultDesc);
	AEDisposeDesc(&resultDesc);
#if useLog
	printf("end putBoolToReply\n");
#endif
	return err;
}

OSErr putMissingValueToReply(AppleEvent *reply)
{
	OSErr err;
	DescType resultType = 'msng';
	AEDesc resultDesc;
	err=AECreateDesc(resultType, NULL, 0, &resultDesc);
	err=AEPutParamDesc(reply, keyAEResult, &resultDesc);
	AEDisposeDesc(&resultDesc);
	return err;
}

OSErr putAliasToReply(AliasHandle inAlias, AppleEvent *reply)
{
	OSErr err;
	AEDesc resultDesc;
	HLock((Handle)inAlias);
	err = AECreateDesc(typeAlias, (Ptr) (*inAlias),
							GetHandleSize((Handle) inAlias), &resultDesc);
	HUnlock((Handle)inAlias);
	err=AEPutParamDesc(reply, keyAEResult, &resultDesc);
	AEDisposeDesc(&resultDesc);
	return err;
}

OSErr putFilePathToReply(CFURLRef inURL, AppleEvent *reply)
{	
	OSErr err;
	char buffer[bufferSize];
	CFURLGetFileSystemRepresentation(inURL, true, (UInt8 *)buffer, bufferSize);
	
	AEDesc resultDesc;
	err=AECreateDesc(typeUTF8Text, buffer, strlen(buffer), &resultDesc);
	
	
	if (err != noErr) goto bail;
	
	err=AEPutParamDesc(reply, keyAEResult, &resultDesc);
	//if (err != noErr) {
	AEDisposeDesc(&resultDesc);
	//}
	
bail:
		return err;
}