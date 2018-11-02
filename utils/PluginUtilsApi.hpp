#ifndef PluginUtilsApi_h
#define PluginUtilsApi_h

#include "base/ccConfig.h"
#include "jsapi.h"
#include "jsfriendapi.h"
#include "platform/android/jni/JniHelper.h"
#include <jni.h>

#include "CallbackFrame.h"

void register_all_utils_framework(JSContext* cx, JS::HandleObject obj);

#endif /* PluginUtilsApi_h */
