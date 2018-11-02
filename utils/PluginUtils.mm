#import "PluginUtilsApi.h"
#include "scripting/js-bindings/manual/cocos2d_specifics.hpp"

int CallbackFrame::nextId = 0;
std::vector<CallbackFrame*> CallbackFrame::callbackVector;

///////////////////////////////////////
//
//  Plugin Utils API
//
///////////////////////////////////////

static void printLog(const char* str) {
    CCLOG("%s", str);
}

static bool jsb_utils_deviceid(JSContext *cx, uint32_t argc, jsval *vp)
{
    printLog("jsb_utils_deviceid");
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
    JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
    if(argc == 0) {
        //std::string result = cocos2d::JniHelper::callStaticStringMethod("org/cocos2dx/javascript/Util", "GetId");
        //rec.rval().set(std_string_to_jsval(cx, result));
        return true;
    } else {
        JS_ReportError(cx, "Invalid number of arguments");
        return false;
    }
}

static bool jsb_utils_deviceidmd5(JSContext *cx, uint32_t argc, jsval *vp)
{
    printLog("jsb_utils_deviceidmd5");
    JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
    JS::RootedObject obj(cx, args.thisv().toObjectOrNull());
    JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
    if(argc == 0) {
        //std::string result = cocos2d::JniHelper::callStaticStringMethod("org/cocos2dx/javascript/Util", "GetIdMd5");
        //rec.rval().set(std_string_to_jsval(cx, result));
        return true;
    } else {
        JS_ReportError(cx, "Invalid number of arguments");
        return false;
    }
}

void register_all_utils_framework(JSContext* cx, JS::HandleObject obj) {
    printLog("[Utils] register js interface");
    JS::RootedObject ns(cx);
    get_or_create_js_obj(cx, obj, "utils", &ns);

    JS_DefineFunction(cx, ns, "DeviceId", jsb_utils_deviceid, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT);
    JS_DefineFunction(cx, ns, "DeviceIdMd5", jsb_utils_deviceidmd5, 0, JSPROP_ENUMERATE | JSPROP_PERMANENT);

}
