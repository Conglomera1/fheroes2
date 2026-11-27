// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "../../../src/fheroes2/castle/conscription.h"

#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace km
{
    enum class Kind { Integer, Decimal, Choice, Toggle };
    struct Setting
    {
        const char * id;
        const char * key;
        Kind kind;
        double minimum;
        double maximum;
        double initial;
        const wchar_t * french;
        const wchar_t * english;
        const wchar_t * frenchHelp;
        const wchar_t * englishHelp;
        const wchar_t * frenchChoices;
        const wchar_t * englishChoices;
    };
    inline constexpr Setting settings[] = {
#include "settings.inc"
    };
    constexpr size_t count = std::size( settings );
    using Values = std::array<double, count>;

    inline constexpr size_t index( const std::string_view id )
    {
        for ( size_t i = 0; i < count; ++i ) {
            if ( settings[i].id == id )
                return i;
        }
        return count;
    }

    inline constexpr auto modeIndex = index( "mode" );
    inline constexpr auto baseIndex = index( "base" );

    inline double defaultValue( const size_t i, const Values & values )
    {
        return i == baseIndex ? Conscription::defaultGoldPerPeasant( static_cast<int32_t>( values[modeIndex] ) ) : settings[i].initial;
    }

    inline bool selectMode( Values & values, const double mode )
    {
        if ( values[modeIndex] == mode )
            return false;

        values[modeIndex] = mode;
        if ( mode != 0 )
            values[baseIndex] = defaultValue( baseIndex, values );
        return true;
    }

    inline Values defaults()
    {
        Values values{};
        for ( size_t i = 0; i < count; ++i )
            values[i] = settings[i].initial;
        return values;
    }

    inline std::string_view trim( const std::string_view value )
    {
        const auto first = value.find_first_not_of( " \t\r\n" );
        return first == value.npos ? std::string_view{} : value.substr( first, value.find_last_not_of( " \t\r\n" ) - first + 1 );
    }

    inline std::string key( const std::string_view line )
    {
        const auto clean = trim( line );
        const auto equal = clean.find( '=' );
        if ( clean.empty() || clean[0] == '#' || clean[0] == ';' || equal == clean.npos )
            return {};
        std::string name( trim( clean.substr( 0, equal ) ) );
        for ( char & c : name ) {
            if ( c >= 'A' && c <= 'Z' )
                c = static_cast<char>( c + ( 'a' - 'A' ) );
        }
        return name;
    }

    inline std::optional<double> parse( std::string value, const Setting & setting )
    {
        std::replace( value.begin(), value.end(), ',', '.' );
        double result = 0;
        const auto last = value.data() + value.size();
        const auto [end, error] = std::from_chars( value.data(), last, result );
        if ( error != std::errc{} || end != last || !std::isfinite( result ) || result < setting.minimum || result > setting.maximum
             || ( setting.kind != Kind::Decimal && result != std::floor( result ) ) )
            return {};
        return result;
    }

    inline std::string format( const double value )
    {
        char buffer[64]{};
        const auto [end, error] = std::to_chars( buffer, buffer + sizeof( buffer ), value, std::chars_format::fixed );
        return error == std::errc{} ? std::string( buffer, end ) : std::string{};
    }

    inline std::string_view withoutBom( std::string_view text )
    {
        if ( text.starts_with( "\xef\xbb\xbf" ) )
            text.remove_prefix( 3 );
        return text;
    }

    inline size_t settingForKey( const std::string_view name )
    {
        for ( size_t i = 0; i < count; ++i ) {
            if ( settings[i].key == name )
                return i;
        }
        return count;
    }

    template <typename Callback>
    void eachLine( std::string_view text, Callback callback )
    {
        while ( !text.empty() ) {
            const size_t newline = text.find( '\n' );
            const auto line = text.substr( 0, newline );
            callback( line );
            if ( newline == text.npos )
                return;
            text.remove_prefix( newline + 1 );
        }
    }

    // A profile is applied only after every recognized value passes validation.
    inline bool load( const std::string_view text, Values & values, const bool profile )
    {
        Values parsed = values;
        std::array<bool, count> seen{};
        bool invalid = false;
        bool found = false;
        std::optional<double> legacyMode;
        eachLine( withoutBom( text ), [&]( const std::string_view line ) {
            const auto name = key( line );
            const size_t i = settingForKey( name );
            if ( i == count && name != "conscription.enabled" )
                return;
            auto value = trim( line.substr( line.find( '=' ) + 1 ) );
            if ( profile )
                value = trim( value.substr( 0, value.find_first_of( "#;" ) ) );
            if ( i == count ) {
                const auto oldMode = parse( std::string( value ), settings[modeIndex] );
                if ( !legacyMode && oldMode && *oldMode <= 1 )
                    legacyMode = *oldMode == 0 ? 0 : Conscription::defaultMode;
                return;
            }
            if ( seen[i] )
                return;
            seen[i] = true;
            found = true;
            if ( settings[i].id == std::string_view( "success" ) ) {
                Setting ratio = settings[i];
                ratio.maximum = 1;
                const auto number = parse( std::string( value ), ratio );
                if ( number )
                    parsed[i] = *number * 100;
                else
                    invalid = true;
            }
            else {
                const auto number = parse( std::string( value ), settings[i] );
                if ( number )
                    parsed[i] = *number;
                else
                    invalid = true;
            }
        } );
        if ( !seen[modeIndex] && legacyMode ) {
            parsed[modeIndex] = *legacyMode;
            found = true;
        }
        if ( profile && ( invalid || !found ) )
            return false;
        if ( !seen[baseIndex] && ( !profile || seen[modeIndex] || legacyMode ) )
            parsed[baseIndex] = defaultValue( baseIndex, parsed );
        values = parsed;
        return true;
    }

    inline std::string updateIni( const std::string_view existing, const Values & values )
    {
        const std::string_view newline = existing.find( "\r\n" ) != existing.npos ? "\r\n" : "\n";
        std::array<bool, count> written{};
        std::string output;
        const auto append = [&]( const size_t i ) {
            const double value = settings[i].id == std::string_view( "success" ) ? values[i] / 100 : values[i];
            output += std::string( settings[i].key ) + " = " + format( value );
            output += newline;
        };
        eachLine( withoutBom( existing ), [&]( std::string_view line ) {
            if ( line.ends_with( '\r' ) )
                line.remove_suffix( 1 );
            const auto i = settingForKey( key( line ) );
            if ( i == count ) {
                output += line;
                output += newline;
            }
            else if ( !written[i] ) {
                append( i );
                written[i] = true;
            }
        } );
        for ( size_t i = 0; i < count; ++i ) {
            if ( !written[i] )
                append( i );
        }
        return output;
    }

    inline uint64_t quote( const Values & values, const uint32_t quantity )
    {
        Conscription::Settings rules;
        rules.mode = 2;
        rules.goldPerPeasant = static_cast<uint32_t>( values[index( "base" )] );
        rules.goldIncrement = values[index( "increment" )];
        rules.goldIncrementStep = static_cast<uint32_t>( values[index( "step" )] );
        rules.goldIncrementMultiplier = values[index( "multiplier" )];
        rules.uniformBatchPrice = values[index( "uniform" )] != 0;
        return Conscription::trainingCost( rules, quantity );
    }
}
