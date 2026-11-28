// SPDX-License-Identifier: GPL-2.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "settings_files.h"

#include <cassert>
#include <filesystem>
#include <iostream>

int main()
{
    namespace files = km::files;
    namespace fs = std::filesystem;

    wchar_t temp[MAX_PATH]{};
    wchar_t name[MAX_PATH]{};
    assert( GetTempPathW( MAX_PATH, temp ) != 0 );
    assert( GetTempFileNameW( temp, L"kms", 0, name ) != 0 );
    assert( DeleteFileW( name ) );
    const fs::path directory( name );
    files::ensureDirectory( directory );
    files::ensureDirectory( directory );
    assert( files::isDirectory( directory ) && !files::isFile( directory ) );

    const fs::path ini = directory / L"knight_mod.ini";
    const fs::path profile = directory / L"Réglages.INI";
    assert( files::isIniProfile( profile ) );
    assert( !files::isIniProfile( directory / L"settings.exe" ) );
    assert( !files::isIniProfile( directory / L"settings.txt:profile.ini" ) );
    assert( !files::isFile( ini ) && files::sameFile( ini, directory / L"KNIGHT_MOD.INI" ) );
    files::replaceText( ini, "original settings\n" );
    assert( files::isFile( ini ) && !files::isDirectory( ini ) );
    files::replaceText( profile, "new profile\n" );
    assert( !files::sameFile( ini, profile ) );
    assert( files::readText( profile ) == "new profile\n" );

    const std::string maxSize( files::maxProfileSize, 'x' );
    files::replaceText( profile, maxSize );
    assert( files::readText( profile ) == maxSize );
    files::replaceText( profile, maxSize + "x" );
    bool tooLarge = false;
    try {
        files::readText( profile );
    }
    catch ( const files::FileTooLarge & ) {
        tooLarge = true;
    }
    assert( tooLarge );

    const HANDLE lock = CreateFileW( ini.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    assert( lock != INVALID_HANDLE_VALUE );
    bool blocked = false;
    try {
        files::replaceText( ini, "new settings\n" );
    }
    catch ( const std::runtime_error & ) {
        blocked = true;
    }
    assert( CloseHandle( lock ) );
    assert( blocked && files::readText( ini ) == "original settings\n" );
    WIN32_FIND_DATAW data{};
    const HANDLE pending = FindFirstFileW( ( directory / L"km-settings-*.tmp" ).c_str(), &data );
    assert( pending == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND );
    files::replaceText( ini, "new settings\n" );
    assert( files::readText( ini ) == "new settings\n" );

    assert( DeleteFileW( profile.c_str() ) );
    assert( DeleteFileW( ini.c_str() ) );
    assert( RemoveDirectoryW( directory.c_str() ) );
    std::cout << "PASS INI profile format, read bounds, atomic writes and temporary file cleanup\n";
}
