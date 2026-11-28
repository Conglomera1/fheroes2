// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace km::files
{
    inline constexpr std::size_t maxProfileSize = 1024 * 1024;

    class FileTooLarge : public std::runtime_error
    {
    public:
        FileTooLarge()
            : std::runtime_error( "Settings file is too large" )
        {}
    };

    std::filesystem::path configurationDirectory();
    std::filesystem::path installationDirectory();
    bool isFile( const std::filesystem::path & path );
    bool isDirectory( const std::filesystem::path & path );
    void ensureDirectory( const std::filesystem::path & path );
    bool isIniProfile( const std::filesystem::path & path );
    bool sameFile( const std::filesystem::path & first, const std::filesystem::path & second );
    std::string readText( const std::filesystem::path & path, std::size_t limit = maxProfileSize );
    void replaceText( const std::filesystem::path & path, std::string_view contents );
    bool launchGame( const std::filesystem::path & installation );
}
