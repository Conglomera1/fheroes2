// Structured observations reuse the existing opt-in diagnostic switch.
#pragma once
#include "taxpayer_test.h"
#include "resource.h"
#include "world.h"
#include "kingdom.h"
#include "conscription.h"
#include <chrono>
#include <set>
#include <sstream>

namespace Economy
{
    template <typename T, size_t N>
    inline std::string arrayJson( const std::array<T, N> & values )
    {
        std::ostringstream out;
        out << '[';
        for ( size_t i = 0; i < N; ++i ) { if ( i ) out << ','; out << values[i]; }
        out << ']';
        return out.str();
    }

    inline std::string fundsJson( const Funds & funds )
    {
        std::ostringstream out;
        out << "{\"gold\":" << funds.gold << ",\"wood\":" << funds.wood << ",\"ore\":" << funds.ore
            << ",\"mercury\":" << funds.mercury << ",\"sulfur\":" << funds.sulfur << ",\"crystal\":" << funds.crystal
            << ",\"gems\":" << funds.gems << '}';
        return out.str();
    }

    struct TraceState
    {
        uint64_t run = static_cast<uint64_t>( std::chrono::system_clock::now().time_since_epoch().count() );
        uint64_t segment = 1, sequence = 0;
        bool pending = true;
        std::string reason = "process_start", source;
        const char * cause = "unattributed";
        int64_t subject = -1;
        std::set<int> openings;
    };

    inline TraceState & traceState()
    {
        static thread_local TraceState state;
        return state;
    }

    inline std::string jsonQuote( const std::string & value )
    {
        const char * hex = "0123456789abcdef";
        std::string result = "\"";
        for ( const unsigned char byte : value ) {
            if ( byte == '"' || byte == '\\' ) { result += '\\'; result += static_cast<char>( byte ); }
            else if ( byte < 32 || byte >= 128 ) {
                // Escape source bytes too: Windows paths need not be UTF-8.
                result += "\\u00";
                result += hex[byte >> 4]; result += hex[byte & 15];
            }
            else result += static_cast<char>( byte );
        }
        return result + '"';
    }

    inline void beginTraceSegment( const std::string & reason, const std::string & source = {} )
    {
        if ( fheroes2::taxpayerSimulationDepth != 0 ) return;
        auto & state = traceState();
        ++state.segment;
        state.pending = true;
        state.reason = reason; state.source = source;
        state.openings.clear();
    }

    class CauseScope
    {
    public:
        CauseScope( const char * cause, const int64_t subject )
            : previousCause( traceState().cause ), previousSubject( traceState().subject )
        { traceState().cause = cause; traceState().subject = subject; }
        ~CauseScope() { traceState().cause = previousCause; traceState().subject = previousSubject; }
        CauseScope( const CauseScope & ) = delete;
        CauseScope & operator=( const CauseScope & ) = delete;
    private:
        const char * previousCause;
        int64_t previousSubject;
    };

    inline std::string traceContext()
    {
        auto & state = traceState();
        std::ostringstream out;
        out << "\"schema\":2,\"run\":\"" << state.run << "\",\"segment\":" << state.segment << ",\"seq\":" << ++state.sequence
            << ",\"day\":" << world.CountDay() << ",\"weekday\":" << ( world.CountDay() ? ( world.CountDay() - 1 ) % 7 + 1 : 0 )
            << ",\"week\":" << ( world.CountDay() ? ( ( world.CountDay() - 1 ) / 7 ) % 4 + 1 : 0 )
            << ",\"month\":" << ( world.CountDay() ? ( world.CountDay() - 1 ) / 28 + 1 : 0 )
            << ",\"cause\":" << jsonQuote( state.cause ) << ",\"subject\":" << state.subject;
        return out.str();
    }

    inline void ensureTraceSegment()
    {
        auto & state = traceState();
        if ( !state.pending ) return;
        state.pending = false;
        const auto & config = Conscription::settings();
        COUT( Logging::GetTimeString() << ": [KNIGHT ECONOMY] {" << traceContext() << ",\"event\":\"SESSION_BEGIN\",\"reason\":" << jsonQuote( state.reason )
              << ",\"source_path_bytes\":" << jsonQuote( state.source ) << ",\"map_seed\":" << world.GetMapSeed()
              << ",\"peasant_cost\":" << Monster( Monster::PEASANT ).GetCost().gold << ",\"tax_rate\":" << Kingdom::GetTaxpayerGoldPerPeasantPerDay()
              << ",\"require_farms\":" << ( Kingdom::TaxpayerRequiresFarms() ? "true" : "false" )
              << ",\"enabled\":" << ( config.enabled ? "true" : "false" ) << ",\"odd_limit\":" << config.oddDayLimit << ",\"even_limit\":" << config.evenDayLimit
              << ",\"success_scaled\":" << config.successRate << ",\"fee\":" << config.goldPerPeasant
              << ",\"composition\":[" << config.quotas[0] << ',' << config.quotas[1] << ',' << config.quotas[2] << ']'
              << ",\"prerequisites\":" << config.requiredBuildings << ",\"tg_cost\":[" << config.trainingGold << ',' << config.trainingWood << ',' << config.trainingOre << ']'
              << ",\"farm_cost\":[" << config.farmGold << ',' << config.farmWood << ',' << config.farmOre << "]}" );
    }
}

#define ECONOMY_TRACE( level, event, fields ) \
    do { \
        if ( Settings::Get().taxpayerTestDiagnostics() >= ( level ) && fheroes2::taxpayerSimulationDepth == 0 ) { \
            Economy::ensureTraceSegment(); \
            COUT( Logging::GetTimeString() << ": [KNIGHT ECONOMY] {" << Economy::traceContext() \
                  << ",\"event\":\"" << event << "\"" << fields << '}' ); \
        } \
    } while ( false )

namespace Economy
{
    inline void observeOpeningFunds( const int player, const Funds & funds )
    {
        if ( Settings::Get().taxpayerTestDiagnostics() < 1 || fheroes2::taxpayerSimulationDepth != 0 ) return;
        ensureTraceSegment();
        if ( traceState().openings.insert( player ).second ) {
            ECONOMY_TRACE( 1, "OPENING_BALANCE", ",\"player\":" << player << ",\"funds\":" << fundsJson( funds ) );
        }
    }
}
