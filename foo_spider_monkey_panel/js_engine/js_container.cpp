#include <stdafx.h>
#include "js_container.h"

#include <js_objects/global_object.h>
#include <js_objects/gdi_graphics.h>
#include <js_utils/js_error_helper.h>


// TODO: remove js_panel_window

namespace mozjs
{

JsContainer::JsContainer()    
{
    pJsCtx_ = nullptr;
    nativeGraphics_ = NULL;
    jsStatus_ = Mjs_NotPrepared;
}

JsContainer::~JsContainer()
{
    Finalize();
    pJsCtx_ = nullptr;
}

bool JsContainer::Prepare( JSContext *cx, js_panel_window& parentPanel )
{
    assert( cx );

    pJsCtx_ = cx;
    pParentPanel_ = &parentPanel;
    jsStatus_ = Mjs_Prepared;

    return Initialize();
}

bool JsContainer::Initialize()
{
    assert( Mjs_NotPrepared != jsStatus_ );
    assert( pJsCtx_ );
    assert( pParentPanel_ );

    if ( Mjs_Ready == jsStatus_ )
    {
        return true;
    }

    if ( jsGlobal_.initialized() || jsGraphics_.initialized() )
    {
        jsGraphics_.reset();
        jsGlobal_.reset();
    }

    JSAutoRequest ar( pJsCtx_ );

    jsGlobal_.init( pJsCtx_, JsGlobalObject::Create( pJsCtx_, *this, *pParentPanel_ ) );
    if ( !jsGlobal_ )
    {
        return false;
    }

    JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

    jsGraphics_.init( pJsCtx_, JsGdiGraphics::Create( pJsCtx_ ) );
    if ( !jsGraphics_ )
    {
        jsGlobal_.reset();
        return false;
    }

    nativeGraphics_ = static_cast<JsGdiGraphics*>(JS_GetPrivate( jsGraphics_ ));
    assert( nativeGraphics_ );

    jsStatus_ = Mjs_Ready;
    return true;
}

void JsContainer::Finalize()
{
    if ( Mjs_NotPrepared == jsStatus_ 
         || Mjs_Prepared == jsStatus_ )
    {
        return;
    }

    if ( Mjs_Failed != jsStatus_ )
    {// Don't supress error: it should be cleared only on initialization
        jsStatus_ = Mjs_Prepared;
    }
    
    nativeGraphics_ = nullptr;
    jsGraphics_.reset();
    jsGlobal_.reset();
}

void JsContainer::Fail()
{
    Finalize();
    jsStatus_ = Mjs_Failed;
}

bool JsContainer::ExecuteScript( std::string_view scriptCode )
{
    assert( pJsCtx_ );
    assert( jsGlobal_.initialized() );
    assert( Mjs_Ready == jsStatus_ );

    JSAutoRequest ar( pJsCtx_ );
    JSAutoCompartment ac( pJsCtx_, jsGlobal_ );

    JS::CompileOptions opts( pJsCtx_ );
    opts.setFileAndLine( "<main>", 1 );

    JS::RootedValue rval( pJsCtx_ );

    AutoReportException are( pJsCtx_ );
    bool bRet = JS::Evaluate( pJsCtx_, opts, scriptCode.data(), scriptCode.length(), &rval );
    if ( !bRet )
    {
        console::printf( JSP_NAME "JS::Evaluate failed\n" );
        return false;
    }

    return true;
}

JsContainer::JsStatus JsContainer::GetStatus() const
{
    return jsStatus_;
}

JS::HandleObject JsContainer::GetGraphics() const
{
    return jsGraphics_;
}

JsContainer::GraphicsWrapper::GraphicsWrapper( JsContainer& parent, Gdiplus::Graphics& gr )
    :parent_( parent )
{
    assert( Mjs_Ready == parent_.jsStatus_ );
    assert( parent_.nativeGraphics_ );

    parent_.nativeGraphics_->SetGraphicsObject( &gr );
}

JsContainer::GraphicsWrapper::~GraphicsWrapper()
{
    if ( Mjs_Ready == parent_.jsStatus_ )
    {
        parent_.nativeGraphics_->SetGraphicsObject( nullptr );
    }
}

}