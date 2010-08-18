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


CFMutableArrayRef CFMutableArrayCreatePOSIXPathsWithEvent(
								  const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr)
{
	CFMutableArrayRef outArray = NULL;
	DescType typeCode;
	Size dataSize;
	AEDescList  aeList;
	AECreateDesc(typeNull, NULL, 0, &aeList);
	
	*errPtr = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((*errPtr != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	*errPtr = AEGetParamDesc(ev, theKey, typeAEList, &aeList);
	if (*errPtr != noErr) goto bail;
	
    long count = 0;
	*errPtr = AECountItems(&aeList, &count);
	if (*errPtr != noErr) goto bail;
	
	outArray = CFArrayCreateMutable(NULL, count, &kCFTypeArrayCallBacks);
	
	for(long index = 1; index <= count; index++) {
		void *value_ptr = NULL;
		Size data_size;
		*errPtr = AEGetNthPtr(&aeList, index, typeFileURL,
						  NULL, NULL, value_ptr,
						  0, &data_size);
		if (*errPtr == noErr) {
			value_ptr = malloc(data_size);
			*errPtr = AEGetNthPtr(&aeList, index, typeFileURL,
							  NULL, NULL, value_ptr,
							  data_size, NULL);
		}
		if (*errPtr != noErr) {
			fputs("Fail to AEGetNthPtr in CFMutableArrayCreatePOSIXPathsWithEvent", stderr);
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
		CFArrayAppendValue(outArray, path);		
		CFRelease(file_url);
		CFRelease(path);
		free(value_ptr);
    }
bail:
	AEDisposeDesc(&aeList);
	return outArray;
}

/* deprecated */
OSErr getPOSIXPathArray(const AppleEvent *ev, AEKeyword theKey,  CFMutableArrayRef *outArray)
{
	OSErr err = noErr;
	*outArray = CFMutableArrayCreatePOSIXPathsWithEvent(ev, theKey, &err);
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
	AEDisposeDesc(&givenDesc);
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
	AEDisposeDesc(&givenDesc);
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
	AEDescList aeList;
	AECreateDesc(typeNull, NULL, 0, &aeList);
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((err != noErr) || (typeCode == typeNull)){
		fputs("Failed to AESizeOfParam in getFloatArray", stderr);
		goto bail;
	}
	
	err = AEGetParamDesc(ev, theKey, typeAEList, &aeList);
	if (err != noErr) {
		fputs("Failed to AEGetParamDesc in getFloatArray", stderr);
		goto bail;
	}
    long count = 0;
	err = AECountItems(&aeList, &count);
	if (err != noErr) {
		fputs("Failed to AECountItems in getFloatArray", stderr);
		goto bail;
	}
	*outArray = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	
    for(long index = 1; index <= count; index++) {
		float value;
		err = AEGetNthPtr(&aeList, index, typeIEEE32BitFloatingPoint,
						  NULL, NULL, &value,
						  sizeof(value), NULL);
		if (err != noErr) {
			fputs("Fail to AEGetNthPtr in getFloatArray", stderr);
			goto bail;
		}
		CFNumberRef cfnum = CFNumberCreate(NULL, kCFNumberFloat32Type, &value);
		CFArrayAppendValue(*outArray, cfnum);
		CFRelease(cfnum);
    }
bail:
	AEDisposeDesc(&aeList);
#if useLog
	CFShow(*outArray);
	fputs("end of getFloatArray", stderr);
#endif	
	return err;
}

CFURLRef CFURLCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr)
{
	CFURLRef file_url = NULL;
	Size data_size;
	void *value_ptr = NULL;
	
	*errPtr = AEGetParamPtr(ev, theKey, typeFileURL, NULL, NULL, 0, &data_size);
	if (*errPtr != noErr) goto bail;
	value_ptr = malloc(data_size);
	if (value_ptr == NULL) {
		fputs("Faild to malloc in CFStringCreatePOSIXPathWithEvent", stderr);
		goto bail;
	}
	*errPtr = AEGetParamPtr(ev, theKey, typeFileURL, NULL, value_ptr, data_size, NULL);
	if (*errPtr != noErr) {
		free(value_ptr);
		goto bail;
	}
	
	file_url = CFURLCreateAbsoluteURLWithBytes(NULL,
											(const UInt8 *)value_ptr,
											data_size,
											kCFStringEncodingUTF8,
											NULL,
											false);
	free(value_ptr);
bail:
	return file_url;
}

CFStringRef CFStringCreateWithEvent(const AppleEvent *ev, AEKeyword theKey, OSErr *errPtr)
{
	CFStringRef outStr = NULL;
	DescType typeCode;
	DescType returnedType;
    Size actualSize;
	Size dataSize;
	CFStringEncoding encodeKey;
	OSType a_type;
	
	*errPtr = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if ((*errPtr != noErr) || (typeCode == typeNull)){
		goto bail;
	}
	
	if (dataSize == 0) {
		outStr = CFSTR("");
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
			*errPtr = AEGetParamPtr(ev, theKey, typeCode, &returnedType, &a_type, dataSize, &actualSize);
			if (a_type == cMissingValue) {
				goto bail;
			}
			//break;
		default :
			typeCode = typeUnicodeText;
			encodeKey = kCFStringEncodingUnicode;
	}
	
	UInt8 *dataPtr = malloc(dataSize);
	if (dataPtr == NULL) {
		*errPtr = errAEEventFailed;
		fputs("Failed to malloc.", stderr);
		goto bail;
	}
	*errPtr = AEGetParamPtr(ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	if (*errPtr != noErr) {
		free(dataPtr);
		goto bail;
	}
	if (actualSize > dataSize) {
#if useLog
		printf("buffere size is allocated. data:%i actual:%i\n", dataSize, actualSize);
#endif	
		dataSize = actualSize;
		dataPtr = (UInt8 *)realloc(dataPtr, dataSize);
		if (dataPtr == NULL) {
			fputs("Failed to reallocate memory", stderr);
			goto bail;
		}
		*errPtr = AEGetParamPtr(ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	}
	
	if (*errPtr == noErr) {
		outStr = CFStringCreateWithBytes(NULL, dataPtr, dataSize, encodeKey, false);
	}
	
	free(dataPtr);
bail:
	return outStr;	
}

/* deprecated */
OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr)
{
	OSErr err = noErr;
	*outStr = CFStringCreateWithEvent(ev, theKey, &err);
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
	
	if (typeCode == typeType) {
		OSType type_data;
		err = AEGetParamPtr(ev, theKey, typeCode, NULL, &type_data, dataSize, NULL);
		if (err != noErr) {
			fputs("Failed to AEGetParamPtr in isMissingValue", stderr);
			goto bail;
		}
		if (type_data == cMissingValue) {
			*ismsng = true;
		}
	}
	if (typeCode == cMissingValue) {
		*ismsng = true;
	}
	
bail:
	return noErr;
}

OSErr AEDescCreateUTF8Text(CFStringRef string, AEDesc* outDescPtr)
{
	OSErr err = noErr;
	CFStringEncoding kEncoding = kCFStringEncodingUTF8;
	DescType resultType = typeUTF8Text;
	const char *constBuff = CFStringGetCStringPtr(string, kEncoding);
	if (constBuff == NULL) {
		char *buffer;
		CFIndex charLen = CFStringGetLength(string);
		CFIndex maxLen = CFStringGetMaximumSizeForEncoding(charLen, kEncoding)+1; // +1 is for null termination.
		buffer = malloc(sizeof(char)*maxLen);
		CFStringGetCString(string, buffer, maxLen, kEncoding);
		err = AECreateDesc(resultType, buffer, strlen(buffer), outDescPtr);
		free(buffer);
	}
	else {
		err=AECreateDesc(resultType, constBuff, strlen(constBuff), outDescPtr);
	}
	
	return err;
}

OSErr AEDescCreateUnicodeText(CFStringRef string, AEDesc* outDescPtr)
{
	OSErr err = noErr;
	DescType resultType = typeUnicodeText;
	
	UniCharCount length = CFStringGetLength(string);
	const UniChar *constBuff = CFStringGetCharactersPtr(string);
	if (constBuff == NULL) {
		UniChar *buffer;
		buffer = malloc(sizeof(UniChar)*length);
		CFStringGetCharacters(string, CFRangeMake(0, length), buffer);
		err = AECreateDesc(resultType, buffer, sizeof(UniChar)*length, outDescPtr);
		free(buffer);
	}
	else {
		err=AECreateDesc(resultType, constBuff, length*sizeof(UniChar), outDescPtr);
	}
	
	return err;
}

OSErr AEDescCreateWithCFString(CFStringRef string, CFStringEncoding kEncoding, AEDesc* outDescPtr)
{
	// kEncoding can be omitted to specify with giving NULL
	OSErr err;
	DescType resultType;

	switch (kEncoding) {
		case kCFStringEncodingUTF8 :
			resultType = typeUTF8Text;
			err = AEDescCreateUTF8Text(string, outDescPtr);
			break;
		default :
			resultType = typeUnicodeText;
			err = AEDescCreateUnicodeText(string, outDescPtr);
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
		err = AEDescCreateWithCFString(string, kEncoding, &string_desc);
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
	fputs("start putStringToEvent", stderr);
#endif
	OSErr err;	
	AEDesc resultDesc;
	err = AEDescCreateWithCFString(inStr, kEncoding, &resultDesc);
	if (err != noErr) goto bail;
	
	err = AEPutParamDesc(ev, keyword, &resultDesc);
	AEDisposeDesc(&resultDesc);
bail:
#if useLog
	fputs("end putStringToEvent", stderr);
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
	const static DescType missingValue = cMissingValue;
	OSErr err;
	AEDesc resultDesc;
	AECreateDesc(typeType, &missingValue, sizeof(missingValue), &resultDesc);
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

#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5
OSErr sourceStringOfAEDesc(ComponentInstance component, AEDesc* inDesc, AEDesc *outDesc)
{
	OSErr err = noErr;
	OSAID script_id = kOSANullScript;
	CFAttributedStringRef source_text = NULL;
	err = OSACoerceFromDesc(component, inDesc, kOSAModeNull, &script_id);
	if (noErr != err) goto bail;
	err = OSACopySourceString(component, script_id, kOSAModeNull, &source_text);
	if (noErr != err) goto bail;
	CFStringRef plain_text = CFAttributedStringGetString(source_text);
	CFShow(plain_text);
	err = AEDescCreateWithCFString(plain_text, kCFStringEncodingUTF8, outDesc);
bail:
	OSADispose(component, script_id);
	safeRelease(source_text);
	return err;
}
#endif