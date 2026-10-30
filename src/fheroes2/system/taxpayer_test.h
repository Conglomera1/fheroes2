// Diagnostics and observer controls for explicitly enabled Taxpayer test builds.
#pragma once

#include "logging.h"
#include "settings.h"

#define TAXPAYER_TRACE( level, message ) \
    do { \
        if ( Settings::Get().taxpayerTestDiagnostics() >= ( level ) ) { \
            COUT( Logging::GetTimeString() << ": [TAXPAYER DIAG] " << message ); \
        } \
    } while ( false )

#include <utility>
#include <set>
#include <string>

namespace fheroes2
{
    inline bool taxpayerTraceOncePerDay( const int day, const std::string & key )
    {
        static int lastDay = -1;
        static std::set<std::string> seen;
        if ( day != lastDay ) {
            seen.clear();
            lastDay = day;
        }
        return seen.insert( key ).second;
    }

    template <typename Callback>
    class TaxpayerTraceScope
    {
    public:
        explicit TaxpayerTraceScope( Callback callback )
            : _callback( std::move( callback ) )
        {}
        ~TaxpayerTraceScope()
        {
            if ( Settings::Get().taxpayerTestDiagnostics() > 0 ) {
                _callback();
            }
        }
        TaxpayerTraceScope( const TaxpayerTraceScope & ) = delete;
        TaxpayerTraceScope & operator=( const TaxpayerTraceScope & ) = delete;

    private:
        Callback _callback;
    };
}
