// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "game_default.h"
#include "settings.h"

namespace km
{
    inline std::string updateDocumentedIni( const std::string_view existing, const Values & values )
    {
        constexpr std::string_view oldHeader = "# Knight Mod settings\r\n";
        bool useTemplate = trim( existing ).empty();
        if ( existing.starts_with( oldHeader ) ) {
            auto previous = defaults();
            load( existing, previous, false );
            // The old launcher wrote this exact layout. Treat other files as user-edited.
            useTemplate = existing == updateIni( oldHeader, previous );
        }

        const std::string_view source = useTemplate ? gameDefaultIni : existing;
        const std::string_view newline = useTemplate || source.find( "\r\n" ) != source.npos ? "\r\n" : "\n";
        std::string result;
        if ( source.starts_with( "\xef\xbb\xbf" ) )
            result = "\xef\xbb\xbf";

        std::array<bool, count> written{};
        eachLine( withoutBom( source ), [&]( std::string_view line ) {
            if ( line.ends_with( '\r' ) )
                line.remove_suffix( 1 );
            const size_t i = settingForKey( key( line ) );
            if ( i == count || written[i] ) {
                result += line;
            }
            else {
                written[i] = true;
                auto numeric = settings[i];
                double value = values[i];
                if ( numeric.id == std::string_view( "success" ) ) {
                    numeric.maximum = 1;
                    value /= 100;
                }
                const size_t field = line.find( '=' ) + 1;
                const auto tail = line.substr( field );
                const auto fieldText = tail.substr( 0, tail.find_first_of( "#;" ) );
                const auto current = trim( fieldText );
                const auto parsed = parse( std::string( current ), numeric );
                if ( parsed && *parsed == value ) {
                    result += line;
                }
                else {
                    const auto offset = current.empty() ? field + fieldText.size() : static_cast<size_t>( current.data() - line.data() );
                    result += line.substr( 0, offset );
                    const auto formatted = format( value );
                    result += formatted;
                    if ( numeric.kind == Kind::Decimal && formatted.find( '.' ) == std::string::npos )
                        result += ".0";
                    result += line.substr( offset + current.size() );
                }
            }
            result += newline;
        } );
        for ( size_t i = 0; i < count; ++i ) {
            if ( written[i] )
                continue;
            const double value = settings[i].id == std::string_view( "success" ) ? values[i] / 100 : values[i];
            result += settings[i].key;
            result += " = ";
            result += format( value );
            result += newline;
        }
        return result;
    }
}
