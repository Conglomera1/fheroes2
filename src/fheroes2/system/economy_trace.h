// Structured observations reuse the existing opt-in diagnostic switch.
#pragma once
#include "taxpayer_test.h"
#include "resource.h"
#include "world.h"
#include <sstream>

namespace Economy
{
    inline std::string fundsJson( const Funds & funds )
    {
        std::ostringstream out;
        out << "{\"gold\":" << funds.gold << ",\"wood\":" << funds.wood << ",\"ore\":" << funds.ore
            << ",\"mercury\":" << funds.mercury << ",\"sulfur\":" << funds.sulfur << ",\"crystal\":" << funds.crystal
            << ",\"gems\":" << funds.gems << '}';
        return out.str();
    }

    inline uint64_t nextTraceSequence()
    {
        static thread_local uint64_t sequence = 0;
        return ++sequence;
    }
}

#define ECONOMY_TRACE( level, event, fields ) \
    do { \
        if ( Settings::Get().taxpayerTestDiagnostics() >= ( level ) && fheroes2::taxpayerSimulationDepth == 0 ) { \
            COUT( Logging::GetTimeString() << ": [KNIGHT ECONOMY] {\"schema\":1,\"seq\":" << Economy::nextTraceSequence() \
                  << ",\"day\":" << world.CountDay() << ",\"weekday\":" << ( world.CountDay() ? ( world.CountDay() - 1 ) % 7 + 1 : 0 ) \
                  << ",\"event\":\"" << event << "\"" << fields << '}' ); \
        } \
    } while ( false )
