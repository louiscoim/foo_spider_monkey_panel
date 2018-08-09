#include <stdafx.h>
#include "html_window.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/gdi_error_helper.h>
#include <js_utils/winapi_error_helper.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/dispatch_ptr.h>
#include <js_utils/scope_helper.h>

// std::time
#include <ctime>

#pragma warning( push )
#pragma warning(disable: 4192)
#pragma warning(disable: 4146)
#pragma warning(disable: 4278)
#   import <mshtml.tlb>
#   import <shdocvw.dll>
#   undef GetWindowStyle
#   undef GetWindowLong
#   undef GetFreeSpace
#   import <wshom.ocx>
#pragma warning( pop ) 

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
    JsHtmlWindow::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "HtmlWindow",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsHtmlWindow, Close )

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "Close", Close , 0, DefaultPropsFlags() ),
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

const JSClass JsHtmlWindow::JsClass = jsClass;
const JSFunctionSpec* JsHtmlWindow::JsFunctions = jsFunctions;
const JSPropertySpec* JsHtmlWindow::JsProperties = jsProperties;
const JsPrototypeId JsHtmlWindow::PrototypeId = JsPrototypeId::HtmlWindow;

JsHtmlWindow::JsHtmlWindow( JSContext* cx, HtmlWindow2ComPtr pHtaWindow )
    : pJsCtx_( cx )
    , pHtaWindow_( pHtaWindow )
{
}

JsHtmlWindow::~JsHtmlWindow()
{ 
}

std::unique_ptr<JsHtmlWindow> 
JsHtmlWindow::CreateNative( JSContext* cx, const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    const std::wstring wndId = []
    {
        std::srand( unsigned( std::time( 0 ) ) );
        return L"a123";//+ std::to_wstring( std::rand() );
    }();

    MSHTML::IHTMLWindow2Ptr pHtaWindow;
    MSHTML::IHTMLDocument2Ptr pDocument;

    try
    {
        IShellDispatchPtr pShell;
        HRESULT hr = pShell.GetActiveObject( CLSID_Shell );
        if ( FAILED( hr ) )
        {
            hr = pShell.CreateInstance( CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "CreateInstance" );
        }

        const std::wstring features =
            L"singleinstance=yes "
            L"border=dialog "
            L"minimizeButton=no "
            L"maximizeButton=no "
            L"scroll=no "
            L"showintaskbar=yes "
            L"contextMenu=yes "
            L"selection=no "
            L"innerBorder=no";
        const auto htaCode =
            L"<script>moveTo(-1000,-1000);resizeTo(0,0);</script>"
            L"<hta:application id=app " + features + L" />"
            L"<object id='" + wndId + L"' style='display:none' classid='clsid:8856F961-340A-11D0-A96B-00C04FD705A2'>"
            L"    <param name=RegisterAsBrowser value=1>"
            L"</object>";        

        /*
        HINSTANCE dwRet = ShellExecute( nullptr, L"open", L"mshta.exe", htaCode.c_str(), nullptr, 1 );
        if ( (DWORD)dwRet <= 32 )
        {// yep, WinAPI logic
            JS_ReportErrorUTF8( cx, "ShellExecute failed: %u", dwRet );
            return nullptr;
        }
        */

        IWshRuntimeLibrary::IWshShellPtr pWsh;
        hr = pWsh.GetActiveObject( L"WScript.Shell" );
        if ( FAILED( hr ) )
        {
            hr = pWsh.CreateInstance( L"WScript.Shell", nullptr, CLSCTX_INPROC_SERVER );
            IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "CreateInstance" );
        }

        const std::wstring cmd = L"mshta.exe about:" + htaCode;
        hr = pWsh->Run( cmd.c_str() );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "Run" );

        IDispatchPtr pDispatch;
        hr = pShell->Windows( &pDispatch );
        IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "Windows" );

        SHDocVw::IShellWindowsPtr pShellWindows( pDispatch );

        for ( long i = pShellWindows->GetCount() - 1; i >= 0; --i )
        {
            Sleep(100);

            _variant_t va( i, VT_I4 );                       
            CDispatchPtr pCurWindow = pShellWindows->Item( va );
            _variant_t idVal = pCurWindow.Get( L"id" );
            if ( idVal.vt == VT_EMPTY )
            {
                continue;
            }

            const _bstr_t id = static_cast<_bstr_t>(idVal);
            if ( id != _bstr_t(wndId.c_str()) )
            {
                continue;
            }

            pDocument = pCurWindow.Get(L"parent");
            if ( !pDocument )
            {                
                JS_ReportErrorUTF8( cx, "Failed to get IHTMLDocument2" );
                return nullptr;
            }

            pHtaWindow = pDocument->GetparentWindow();
            if ( !pHtaWindow )
            {
                JS_ReportErrorUTF8( cx, "Failed to get IHTMLWindow2" );
                return nullptr;
            }

            break;
        }

        if ( !pHtaWindow )
        {
            JS_ReportErrorUTF8( cx, "Failed to create HTML window" );
            return nullptr;
        }
    }
    catch ( const _com_error& e )
    {
        pfc::string8_fast errorMsg8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.ErrorMessage() );        
        pfc::string8_fast errorSource8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.Source() );
        pfc::string8_fast errorDesc8 = pfc::stringcvt::string_utf8_from_wide( (const wchar_t*)e.Description() );
        JS_ReportErrorUTF8( cx, "COM error: message %s; source: %s; description: %s", errorMsg8.c_str(), errorSource8.c_str(), errorDesc8.c_str() );
        return nullptr;
    }

    _bstr_t bstr = 
        _bstr_t(htmlCode.c_str()) +
        L"<script language=\"JScript\" id=\"" + wndId.c_str() + "\">"
        L"    eval; "
        L"    document.title=\"azaza\";"
        L"    var width = 200;"
        L"    var height = 200;"
        L"    resizeTo(width, height);" +
        L"    moveTo((screen.width-width)/2, (screen.height-height)/2);"
        L"    document.getElementById(\"" + wndId.c_str() + "\").removeNode();"
        L"</script>";

    SAFEARRAY* pSaStrings = SafeArrayCreateVector( VT_VARIANT, 0, 1 );
    scope::final_action autoPsa( [pSaStrings]()
    {
        SafeArrayDestroy( pSaStrings );
    } );
    
    VARIANT *param;
    HRESULT hr = SafeArrayAccessData( pSaStrings, (LPVOID*)&param );
    IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayAccessData" );

    param->vt = VT_BSTR;
    param->bstrVal = bstr.Detach();

    hr = SafeArrayUnaccessData( pSaStrings );
    IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "SafeArrayUnaccessData" );

    hr = pDocument->write( pSaStrings );
    IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "write" );

    hr = pDocument->close();
    IF_HR_FAILED_RETURN_WITH_REPORT( cx, hr, nullptr, "close" );

    return std::unique_ptr<JsHtmlWindow>( new JsHtmlWindow(cx, pHtaWindow ) );
}

size_t JsHtmlWindow::GetInternalSize( const std::wstring& htmlCode, const std::wstring& data, JS::HandleValue callback )
{
    return htmlCode.length() * sizeof( wchar_t ) + data.length() * sizeof( wchar_t );
}

std::optional<nullptr_t> 
JsHtmlWindow::Close()
{
    pHtaWindow_->close();
    return nullptr;
}

}
