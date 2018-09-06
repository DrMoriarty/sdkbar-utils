#ifndef PluginUtilsApi_h
#define PluginUtilsApi_h_

#include "base/ccConfig.h"
#include "jsapi.h"
#include "jsfriendapi.h"

void register_all_utils_framework(JSContext* cx, JS::HandleObject obj);

#endif /* PluginUtilsApi_h_ */
