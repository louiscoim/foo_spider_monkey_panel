#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

namespace mozjs
{

JSObject* GetJsObjectFromValue( JSContext* cx, JS::HandleValue jsValue );

template <typename NativeType>
NativeType* GetNativeFromJsObject( JSContext* cx, JS::HandleObject jsObject )
{
    if ( !jsObject )
    {
        return nullptr;
    }

    const JSClass * jsClass = JS_GetClass( jsObject );
    if ( !jsClass
         || !( jsClass->flags & JSCLASS_HAS_PRIVATE ) )
    {
        return nullptr;
    }

    return static_cast<NativeType *>( JS_GetInstancePrivate( cx, jsObject, &NativeType::GetClass(), nullptr ) );
}

template<typename MozjsObjectType>
void JsFinalizeOp( JSFreeOp* fop, JSObject* obj )
{
    auto x = static_cast<MozjsObjectType*>( JS_GetPrivate( obj ) );
    if ( x )
    {
        delete x;
        JS_SetPrivate( obj, nullptr );
    }
}

}
