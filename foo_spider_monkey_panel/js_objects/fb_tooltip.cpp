#include <stdafx.h>
#include "fb_tooltip.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <utils/winapi_error_helpers.h>
#include <js_utils/js_object_helper.h>

using namespace smp;

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFbTooltip::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbTooltip",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( Activate, JsFbTooltip::Activate )
MJS_DEFINE_JS_FN_FROM_NATIVE( Deactivate, JsFbTooltip::Deactivate )
MJS_DEFINE_JS_FN_FROM_NATIVE( GetDelayTime, JsFbTooltip::GetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetDelayTime, JsFbTooltip::SetDelayTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( SetMaxWidth, JsFbTooltip::SetMaxWidth )
MJS_DEFINE_JS_FN_FROM_NATIVE( TrackPosition, JsFbTooltip::TrackPosition )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Activate", Activate, 0, DefaultPropsFlags() ),
    JS_FN( "Deactivate", Deactivate, 0, DefaultPropsFlags() ),
    JS_FN( "GetDelayTime", GetDelayTime, 1, DefaultPropsFlags() ),
    JS_FN( "SetDelayTime", SetDelayTime, 2, DefaultPropsFlags() ),
    JS_FN( "SetMaxWidth", SetMaxWidth, 1, DefaultPropsFlags() ),
    JS_FN( "TrackPosition", TrackPosition, 2, DefaultPropsFlags() ),
    JS_FS_END
};

MJS_DEFINE_JS_FN_FROM_NATIVE( get_Text, JsFbTooltip::get_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_Text, JsFbTooltip::put_Text )
MJS_DEFINE_JS_FN_FROM_NATIVE( put_TrackActivate, JsFbTooltip::put_TrackActivate )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "Text", get_Text, put_Text, DefaultPropsFlags() ),
    JS_PSGS( "TrackActivate", DummyGetter, put_TrackActivate, DefaultPropsFlags() ),
    JS_PS_END
};

} // namespace

namespace mozjs
{

const JSClass JsFbTooltip::JsClass = jsClass;
const JSFunctionSpec* JsFbTooltip::JsFunctions = jsFunctions;
const JSPropertySpec* JsFbTooltip::JsProperties = jsProperties;
const JsPrototypeId JsFbTooltip::PrototypeId = JsPrototypeId::FbTooltip;

JsFbTooltip::JsFbTooltip( JSContext* cx, HFONT hFont, HWND hParentWnd, HWND hTooltipWnd, std::unique_ptr<TOOLINFO> toolInfo, panel::PanelTooltipParam& p_param_ptr )
    : pJsCtx_( cx )
    , hFont_( hFont )
    , hParentWnd_( hParentWnd )
    , hTooltipWnd_( hTooltipWnd )
    , panelTooltipParam_( p_param_ptr )
    , tipBuffer_( PFC_WIDESTRING( SMP_NAME ) )
    , toolInfo_( std::move( toolInfo ) )
{
    toolInfo_->lpszText = (wchar_t*)tipBuffer_.c_str();

    panelTooltipParam_.hTooltip = hTooltipWnd_;
    panelTooltipParam_.tooltipSize.cx = -1;
    panelTooltipParam_.tooltipSize.cy = -1;
}

JsFbTooltip::~JsFbTooltip()
{
    if ( hTooltipWnd_ && IsWindow( hTooltipWnd_ ) )
    {
        DestroyWindow( hTooltipWnd_ );
    }
    if ( hFont_ )
    {
        DeleteObject( hFont_ );
    }
}

std::unique_ptr<JsFbTooltip>
JsFbTooltip::CreateNative( JSContext* cx, HWND hParentWnd, panel::PanelTooltipParam& p_param_ptr )
{
    SmpException::ExpectTrue( hParentWnd, "Internal error: hParentWnd is null" );

    HWND hTooltipWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hParentWnd,
        nullptr,
        core_api::get_my_instance(),
        nullptr );
    smp::error::CheckWinApi( !!hTooltipWnd, "CreateWindowEx" );

    // Original position
    SetWindowPos( hTooltipWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    std::unique_ptr<TOOLINFO> toolInfo( new TOOLINFO );
    // Set up tooltip information.
    memset( toolInfo.get(), 0, sizeof( TOOLINFO ) );

    toolInfo->cbSize = sizeof( TOOLINFO );
    toolInfo->uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_TRANSPARENT;
    toolInfo->hinst = core_api::get_my_instance();
    toolInfo->hwnd = hParentWnd;
    toolInfo->uId = (UINT_PTR)hParentWnd;
    toolInfo->lpszText = nullptr;

    HFONT hFont = CreateFont(
        -(INT)p_param_ptr.fontSize,
        0,
        0,
        0,
        ( p_param_ptr.fontStyle & Gdiplus::FontStyleBold ) ? FW_BOLD : FW_NORMAL,
        ( p_param_ptr.fontStyle & Gdiplus::FontStyleItalic ) ? TRUE : FALSE,
        ( p_param_ptr.fontStyle & Gdiplus::FontStyleUnderline ) ? TRUE : FALSE,
        ( p_param_ptr.fontStyle & Gdiplus::FontStyleStrikeout ) ? TRUE : FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        p_param_ptr.fontName.c_str() );
    smp::error::CheckWinApi( !!hFont, "CreateFont" );

    SendMessage( hTooltipWnd, TTM_ADDTOOL, 0, (LPARAM)toolInfo.get() );
    SendMessage( hTooltipWnd, TTM_ACTIVATE, FALSE, 0 );
    SendMessage( hTooltipWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM( FALSE, 0 ) );

    return std::unique_ptr<JsFbTooltip>( new JsFbTooltip( cx, hFont, hParentWnd, hTooltipWnd, std::move( toolInfo ), p_param_ptr ) );
}

size_t JsFbTooltip::GetInternalSize( HWND /*hParentWnd*/, const panel::PanelTooltipParam& /*p_param_ptr*/ )
{
    return sizeof( LOGFONT ) + sizeof( TOOLINFO );
}

void JsFbTooltip::Activate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, TRUE, 0 );
}

void JsFbTooltip::Deactivate()
{
    SendMessage( hTooltipWnd_, TTM_ACTIVATE, FALSE, 0 );
}

uint32_t JsFbTooltip::GetDelayTime( uint32_t type )
{
    if ( type < TTDT_AUTOMATIC || type > TTDT_INITIAL )
    {
        throw SmpException( fmt::format( "Invalid delay type: {}", type ) );
    }

    return SendMessage( hTooltipWnd_, TTM_GETDELAYTIME, type, 0 );
}

void JsFbTooltip::SetDelayTime( uint32_t type, int32_t time )
{
    if ( type < TTDT_AUTOMATIC || type > TTDT_INITIAL )
    {
        throw SmpException( fmt::format( "Invalid delay type: {}", type ) );
    }

    SendMessage( hTooltipWnd_, TTM_SETDELAYTIME, type, ( LPARAM )(INT)MAKELONG( time, 0 ) );
}

void JsFbTooltip::SetMaxWidth( uint32_t width )
{
    SendMessage( hTooltipWnd_, TTM_SETMAXTIPWIDTH, 0, width );
}

void JsFbTooltip::TrackPosition( int x, int y )
{
    POINT pt = { x, y };
    ClientToScreen( hParentWnd_, &pt );
    SendMessage( hTooltipWnd_, TTM_TRACKPOSITION, 0, MAKELONG( pt.x, pt.y ) );
}

std::wstring JsFbTooltip::get_Text()
{
    return tipBuffer_;
}

void JsFbTooltip::put_Text( const std::wstring& text )
{
    tipBuffer_ = text;
    toolInfo_->lpszText = (LPWSTR)tipBuffer_.c_str();
    SendMessage( hTooltipWnd_, TTM_SETTOOLINFO, 0, (LPARAM)toolInfo_.get() );
}

void JsFbTooltip::put_TrackActivate( bool activate )
{
    if ( activate )
    {
        toolInfo_->uFlags |= TTF_TRACK | TTF_ABSOLUTE;
    }
    else
    {
        toolInfo_->uFlags &= ~( TTF_TRACK | TTF_ABSOLUTE );
    }

    SendMessage( hTooltipWnd_, TTM_TRACKACTIVATE, activate, (LPARAM)toolInfo_.get() );
}

} // namespace mozjs
