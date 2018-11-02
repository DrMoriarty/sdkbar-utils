#include "scripting/js-bindings/manual/cocos2d_specifics.hpp"
#include "scripting/js-bindings/manual/js_manual_conversions.h"

#ifndef CallbackFrame_h_
#define CallbackFrame_h_

typedef struct CallbackFrame {
    static int nextId;
    static std::vector<CallbackFrame*> callbackVector;
    JSContext *cx;
    mozilla::Maybe<JS::PersistentRootedObject> _ctxObject;
    mozilla::Maybe<JS::PersistentRootedValue> _jsThisObj;
    mozilla::Maybe<JS::PersistentRootedValue> _jsCallback;
    int callbackId;
    CallbackFrame(JSContext *c, JS::RootedObject &ctxOb, JS::HandleValue th, JS::HandleValue func):cx(c)  {
        _ctxObject.construct(cx, ctxOb);
        _jsThisObj.construct(cx, th);
        _jsCallback.construct(cx, func);
        callbackId = nextId++;
        CallbackFrame::pushCallback(this);
    }
    void call(JS::HandleValueArray &args) {
        JSAutoRequest rq(cx);
        JS::RootedValue retVal(cx);
        JS::RootedObject thisObj(cx, _jsThisObj.ref().get().toObjectOrNull());
        JS_CallFunctionValue(cx, thisObj, _jsCallback.ref(), args, &retVal);
    }
    static CallbackFrame* getById(int callbackId) {
        CallbackFrame *cb = NULL;
        for(int i=0; i<callbackVector.size(); i++) {
            if(callbackVector[i]->callbackId == callbackId) {
                cb = callbackVector[i];
                CallbackFrame::callbackVector.erase(callbackVector.begin() + i);
                break;
            }
        }
        return cb;
    }
    static void pushCallback(CallbackFrame* cf) {
        CallbackFrame::callbackVector.push_back(cf);
    }
    ~CallbackFrame() {
        _ctxObject.destroyIfConstructed();
        _jsThisObj.destroyIfConstructed();
        _jsCallback.destroyIfConstructed();
    }
} CallbackFrame;

#endif // CallbackFrame_h_
