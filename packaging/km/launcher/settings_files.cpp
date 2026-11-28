// SPDX-License-Identifier: GPL-2.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>

#include "settings_files.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <memory>

namespace
{
    namespace fs = std::filesystem;

    struct CloseFile
    {
        void operator()( void * handle ) const { CloseHandle( handle ); }
    };

    using FileHandle = std::unique_ptr<void, CloseFile>;
    constexpr DWORD readableSharing = FILE_SHARE_READ | FILE_SHARE_DELETE;

    FileHandle open( const fs::path & file, const DWORD access, const DWORD sharing, const DWORD disposition )
    {
        const HANDLE handle = CreateFileW( file.c_str(), access, sharing, nullptr, disposition, FILE_ATTRIBUTE_NORMAL, nullptr );
        return FileHandle( handle == INVALID_HANDLE_VALUE ? nullptr : handle );
    }

    std::wstring environmentValue( const wchar_t * name )
    {
        const DWORD capacity = GetEnvironmentVariableW( name, nullptr, 0 );
        if ( capacity == 0 )
            return {};
        std::wstring value( capacity, L'\0' );
        const DWORD length = GetEnvironmentVariableW( name, value.data(), capacity );
        if ( length >= capacity )
            return {};
        value.resize( length );
        return value;
    }

    fs::path absolutePath( const fs::path & path )
    {
        const DWORD capacity = GetFullPathNameW( path.c_str(), 0, nullptr, nullptr );
        if ( capacity == 0 )
            throw std::runtime_error( "Get settings path" );
        std::wstring value( capacity, L'\0' );
        const DWORD length = GetFullPathNameW( path.c_str(), capacity, value.data(), nullptr );
        if ( length == 0 || length >= capacity )
            throw std::runtime_error( "Get settings path" );
        value.resize( length );
        return fs::path( value );
    }

    DWORD attributes( const fs::path & path )
    {
        const DWORD result = GetFileAttributesW( path.c_str() );
        if ( result != INVALID_FILE_ATTRIBUTES )
            return result;
        const DWORD error = GetLastError();
        if ( error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND )
            throw std::runtime_error( "Read settings path attributes" );
        return result;
    }
}

fs::path km::files::configurationDirectory()
{
    // Use the same environment precedence as the game.
    const std::wstring home = environmentValue( L"HOME" );
    if ( !home.empty() )
        return absolutePath( fs::path( home ) / L".fheroes2" );
    const std::wstring appData = environmentValue( L"APPDATA" );
    if ( !appData.empty() )
        return absolutePath( fs::path( appData ) / L"fheroes2" );
    wchar_t folder[MAX_PATH]{};
    if ( FAILED( SHGetFolderPathW( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, folder ) ) )
        throw std::runtime_error( "Find user configuration directory" );
    return fs::path( folder ) / L"fheroes2";
}

fs::path km::files::installationDirectory()
{
    std::wstring executable( 32768, L'\0' );
    const auto capacity = static_cast<DWORD>( executable.size() );
    const DWORD length = GetModuleFileNameW( nullptr, executable.data(), capacity );
    if ( length == 0 || length >= capacity )
        throw std::runtime_error( "Find game directory" );
    executable.resize( length );
    return fs::path( executable ).parent_path();
}

bool km::files::isFile( const fs::path & path )
{
    const DWORD flags = attributes( path );
    return flags != INVALID_FILE_ATTRIBUTES && ( flags & FILE_ATTRIBUTE_DIRECTORY ) == 0;
}

bool km::files::isDirectory( const fs::path & path )
{
    const DWORD flags = attributes( path );
    return flags != INVALID_FILE_ATTRIBUTES && ( flags & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}

void km::files::ensureDirectory( const fs::path & path )
{
    if ( SHCreateDirectoryExW( nullptr, path.c_str(), nullptr ) != ERROR_SUCCESS && !isDirectory( path ) )
        throw std::runtime_error( "Create user configuration directory" );
}

bool km::files::isIniProfile( const fs::path & path )
{
    const std::wstring name = path.filename().native();
    return name.find( L':' ) == std::wstring::npos && CompareStringOrdinal( path.extension().c_str(), -1, L".ini", -1, TRUE ) == CSTR_EQUAL;
}

bool km::files::sameFile( const fs::path & first, const fs::path & second )
{
    if ( CompareStringOrdinal( absolutePath( first ).c_str(), -1, absolutePath( second ).c_str(), -1, TRUE ) == CSTR_EQUAL )
        return true;

    constexpr DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const FileHandle left = open( first, 0, sharing, OPEN_EXISTING );
    const FileHandle right = open( second, 0, sharing, OPEN_EXISTING );
    BY_HANDLE_FILE_INFORMATION a{}, b{};
    return left && right && GetFileInformationByHandle( left.get(), &a ) && GetFileInformationByHandle( right.get(), &b )
           && a.dwVolumeSerialNumber == b.dwVolumeSerialNumber && a.nFileIndexHigh == b.nFileIndexHigh && a.nFileIndexLow == b.nFileIndexLow;
}

std::string km::files::readText( const fs::path & path, const std::size_t limit )
{
    const FileHandle file = open( path, GENERIC_READ, readableSharing, OPEN_EXISTING );
    LARGE_INTEGER size{};
    if ( !file || GetFileType( file.get() ) != FILE_TYPE_DISK || !GetFileSizeEx( file.get(), &size ) || size.QuadPart < 0 )
        throw std::runtime_error( "Open settings for reading" );
    if ( static_cast<unsigned long long>( size.QuadPart ) > limit )
        throw FileTooLarge();

    std::string text;
    text.reserve( static_cast<std::size_t>( size.QuadPart ) );
    std::array<char, 16384> buffer;
    for ( ;; ) {
        DWORD bytesRead = 0;
        if ( !ReadFile( file.get(), buffer.data(), static_cast<DWORD>( buffer.size() ), &bytesRead, nullptr ) )
            throw std::runtime_error( "Read settings" );
        if ( bytesRead == 0 )
            return text;
        if ( bytesRead > limit - text.size() )
            throw FileTooLarge();
        text.append( buffer.data(), bytesRead );
    }
}

void km::files::replaceText( const fs::path & path, std::string_view contents )
{
    GUID token{};
    wchar_t name[40]{};
    if ( FAILED( CoCreateGuid( &token ) ) || StringFromGUID2( token, name, static_cast<int>( std::size( name ) ) ) == 0 )
        throw std::runtime_error( "Name temporary settings file" );

    const fs::path temporary = path.parent_path() / ( std::wstring( L"km-settings-" ) + name + L".tmp" );
    FileHandle file = open( temporary, GENERIC_WRITE, 0, CREATE_NEW );
    if ( !file )
        throw std::runtime_error( "Create temporary settings file" );
    try {
        while ( !contents.empty() ) {
            const auto chunk = static_cast<DWORD>( std::min( contents.size(), std::size_t{ 65536 } ) );
            DWORD written = 0;
            if ( !WriteFile( file.get(), contents.data(), chunk, &written, nullptr ) || written == 0 )
                throw std::runtime_error( "Write settings" );
            contents.remove_prefix( written );
        }
        if ( !FlushFileBuffers( file.get() ) )
            throw std::runtime_error( "Flush settings" );
        file.reset();
        if ( !MoveFileExW( temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
            throw std::runtime_error( "Replace settings" );
    }
    catch ( ... ) {
        file.reset();
        DeleteFileW( temporary.c_str() );
        throw;
    }
}

bool km::files::launchGame( const fs::path & installation )
{
    const fs::path executable = installation / L"fheroes2KM.exe";
    std::wstring command = L"\"" + executable.wstring() + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof( startup );
    PROCESS_INFORMATION process{};
    if ( !CreateProcessW( executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, installation.c_str(), &startup, &process ) )
        return false;
    CloseHandle( process.hThread );
    CloseHandle( process.hProcess );
    return true;
}
