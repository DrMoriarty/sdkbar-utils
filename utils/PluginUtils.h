#ifndef _PluginUtils_h_
#define _PluginUtils_h_

#include "utf16string.h"
#include "scripting/js-bindings/manual/cocos2d_specifics.hpp"
#include "scripting/js-bindings/manual/js_manual_conversions.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include "platform/android/jni/JniHelper.h"
#include <jni.h>
#endif
#include <sstream>
#include "external/ConvertUTF/ConvertUTF.h"

typedef int64_t Status;
typedef unsigned char UTF8;
typedef uint32_t UTF32;

int checkUTF8LegalSymbols(const UTF8* sourceStart, const UTF8* sourceEnd);
std::string printUTF8Code(const UTF8* source);
jsval my_std_string_to_jsval(JSContext* cx, const std::string& v);
ConversionResult myUTF8ToUTF16(const std::string& utf8, std::u16string& outUtf16);

std::string Stringify(JSContext *cx, JS::RootedValue &arg0Val);
std::string utf8_from_wstring(const std::wstring& src, Status* err);
std::wstring wstring_from_utf8(const std::string& src, Status* err);

#include "CallbackFrame.h"

#endif // _PluginUtils_h_
