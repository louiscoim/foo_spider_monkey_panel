#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;

namespace Gdiplus
{
class Font;
}

namespace mozjs
{

class JsFbMetadbHandleList
{
public:
    ~JsFbMetadbHandleList();
    
    static JSObject* Create( JSContext* cx, metadb_handle_list_cref handles );

    static const JSClass& GetClass();

    metadb_handle_list_ref GetList();

public: // methods
    std::optional<std::nullptr_t> Add( JS::HandleValue handle );
    std::optional<std::nullptr_t> AddRange( JS::HandleValue handles );
    std::optional<int32_t> BSearch( JS::HandleValue handle );
    std::optional<double> CalcTotalDuration();
    //std::optional<std::nullptr_t> CalcTotalSize( LONGLONG* p );
    std::optional<JSObject*> Clone();
    //std::optional<std::nullptr_t> Convert( VARIANT* p );
    std::optional<int32_t> Find( JS::HandleValue handle );
    //std::optional<std::nullptr_t> GetLibraryRelativePaths( VARIANT* p );
    std::optional<std::nullptr_t> Insert( uint32_t index, JS::HandleValue handle );
    std::optional<std::nullptr_t> InsertRange( uint32_t index, JS::HandleValue handles );
    std::optional<std::nullptr_t> MakeDifference( JS::HandleValue handles );
    std::optional<std::nullptr_t> MakeIntersection( JS::HandleValue handles );
    std::optional<std::nullptr_t> MakeUnion( JS::HandleValue handles );
    //std::optional<std::nullptr_t> OrderByFormat( __interface IFbTitleFormat* script, int direction );
    std::optional<std::nullptr_t> OrderByPath();
    std::optional<std::nullptr_t> OrderByRelativePath();
    std::optional<std::nullptr_t> RefreshStats();
    std::optional<std::nullptr_t> Remove( JS::HandleValue handle );
    std::optional<std::nullptr_t> RemoveAll();
    std::optional<std::nullptr_t> RemoveById( uint32_t index );
    std::optional<std::nullptr_t> RemoveRange( uint32_t from, uint32_t count );
    std::optional<std::nullptr_t> Sort();
    std::optional<std::nullptr_t> UpdateFileInfoFromJSON( std::string str );

public: // props
    std::optional<uint32_t> get_Count();
    std::optional<JSObject*> get_Item( uint32_t index );
    std::optional<std::nullptr_t> put_Item( uint32_t index, JS::HandleValue handle );
    // TODO: add array methods

private:
    JsFbMetadbHandleList( JSContext* cx, metadb_handle_list_cref handles );
    JsFbMetadbHandleList( const JsFbMetadbHandleList& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    metadb_handle_list metadbHandleList_;
};

}