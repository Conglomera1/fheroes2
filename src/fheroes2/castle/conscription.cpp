/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "conscription.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>

#include "army_troop.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "payment.h"
#include "resource.h"
#include "system.h"
#include "tinyconfig.h"
#include "world.h"

const Conscription::Settings & Conscription::settings()
{
    static const Settings value = [] {
        Settings result;
        // Ensure the existing Taxpayer section is created first; preserve all user settings.
        Kingdom::GetTaxpayerGoldPerPeasantPerDay();
        const std::string path = System::concatPath( System::GetConfigDirectory( "fheroes2" ), "knight_mod.ini" );
        TinyConfig config( '=', '#' );
        config.Load( path );
        std::ofstream additions( path, std::ios::app );
        bool headerWritten = false;
        const auto read = [&]( const char * key, const int32_t fallback, const int32_t maximum ) {
            if ( config.Exists( key ) )
                return std::clamp( config.IntParams( key ), 0, maximum );
            if ( additions ) {
                if ( !headerWritten ) {
                    additions << "\n# Conscription prototype. Restart after editing.\n"
                                 "# Ratios: quotas per cycle; at least one must be positive.\n"
                                 "# daily_limit: mandatory formations per town per day (0 disables training).\n"
                                 "# Garrison first; hut formations also pay the normal Peasant recruitment cost.\n";
                    headerWritten = true;
                }
                additions << key << " = " << fallback << '\n';
            }
            return fallback;
        };
        result.goldPerPeasant = read( "conscription.gold_per_peasant", 100, 1000000 );
        result.quotas = { static_cast<uint32_t>( read( "conscription.archers", 8, 10000 ) ),
                          static_cast<uint32_t>( read( "conscription.pikemen", 5, 10000 ) ),
                          static_cast<uint32_t>( read( "conscription.swordsmen", 4, 10000 ) ) };
        if ( result.quotas == Quotas{} ) result.quotas = { 8, 5, 4 };
        result.dailyLimit = read( "conscription.daily_limit", 3, 10000 );
        result.trainingGold = read( "training_grounds.gold", 2000, 1000000 );
        result.trainingWood = read( "training_grounds.wood", 10, 1000000 );
        result.trainingOre = read( "training_grounds.ore", 10, 1000000 );
        result.farmGold = read( "farm_reconversion.gold", 1500, 1000000 );
        result.farmWood = read( "farm_reconversion.wood", 0, 1000000 );
        result.farmOre = read( "farm_reconversion.ore", 0, 1000000 );
        return result;
    }();
    return value;
}

Funds Castle::getBuildingCost( const uint32_t buildingType ) const
{
    if ( _race == Race::KNGT ) {
        const auto & config = Conscription::settings();
        if ( buildingType == BUILD_TRAINING_GROUNDS )
            return Funds( config.trainingOre, config.trainingWood, 0, 0, 0, 0, config.trainingGold );
        if ( buildingType == BUILD_WEL2 && hasTrainingGrounds() )
            return Funds( config.farmOre, config.farmWood, 0, 0, 0, 0, config.farmGold );
    }
    return PaymentConditions::BuyBuilding( _race, buildingType );
}

std::array<uint64_t, 3> Castle::previewConscription( const uint32_t count ) const
{
    auto state = _conscription;
    return Conscription::advance( state, count, Conscription::settings().quotas );
}

std::string Castle::conscriptionPreviewText( const uint32_t count ) const
{
    const auto recruits = previewConscription( count );
    const uint32_t dwellings[] = { DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
    std::vector<std::string> parts;
    for ( size_t i = 0; i < 3; ++i ) {
        if ( recruits[i] ) {
            const Monster monster( _race, GetActualDwelling( dwellings[i] ) );
            parts.push_back( std::to_string( recruits[i] ) + " " + monster.GetPluralName( static_cast<uint32_t>( recruits[i] ) ) );
        }
    }
    std::string text = "This will train ";
    for ( size_t i = 0; i < parts.size(); ++i ) {
        if ( i ) text += i + 1 == parts.size() ? " and " : ", ";
        text += parts[i];
    }
    return text + ".";
}

std::string Castle::conscriptionError( const ArmyTroop & troop ) const
{
    const auto * army = troop.GetArmy();
    const Heroes * guest = world.GetHero( *this );
    if ( !hasTrainingGrounds() || !army || troop.GetID() != Monster::PEASANT || !troop.GetCount()
         || ( army != &_army && ( !guest || army != &guest->GetArmy() ) ) || army->GetColor() != GetColor() )
        return "These Peasants cannot be conscripted here.";
    // Confirm this is an actual stack in the army, not a detached preview troop.
    bool present = false;
    for ( size_t i = 0; i < army->Size(); ++i )
        present = present || army->GetTroop( i ) == &troop;
    if ( !present ) return "These Peasants cannot be conscripted here.";
    if ( army != &_army && army->SaveLastTroop() )
        return "A hero must keep at least one creature. Split this stack or move it to the garrison first.";
    const auto & config = Conscription::settings();
    const uint64_t cost = uint64_t{ troop.GetCount() } * config.goldPerPeasant;
    if ( cost > static_cast<uint64_t>( std::numeric_limits<int32_t>::max() ) )
        return "This stack is too large to conscript. Split it first.";
    if ( !GetKingdom().AllowPayment( Funds( Resource::GOLD, static_cast<uint32_t>( cost ) ) ) )
        return "You can't afford to conscript this stack. Split it to conscript fewer Peasants.";
    const uint32_t already = _conscriptionWeek == world.CountWeek() ? _conscriptionThisWeek : 0;
    if ( config.weeklyLimit && uint64_t{ already } + troop.GetCount() > config.weeklyLimit )
        return "This stack exceeds the town's remaining weekly conscription limit. Split it or wait until next week.";
    if ( uint64_t{ already } + troop.GetCount() > std::numeric_limits<uint32_t>::max()
         || _conscription.total > std::numeric_limits<uint64_t>::max() - troop.GetCount() )
        return "The conscription counter cannot accept this stack.";
    const auto recruits = previewConscription( troop.GetCount() );
    for ( size_t i = 0; i < 3; ++i ) {
        if ( recruits[i] + _dwelling[i + 1] > std::numeric_limits<uint32_t>::max() )
            return "There is no room for these recruits in the town's recruitment pools.";
    }
    return {};
}

bool Castle::conscript( ArmyTroop & troop )
{
    // The old manual action must never bypass the daily formation schedule.
    static_cast<void>( troop );
    return false;
}

void Castle::processDailyConscription( Kingdom & kingdom )
{
    if ( world.CountDay() <= 1 ) return;
    const auto & config = Conscription::settings();
    std::vector<Castle *> towns;
    std::vector<Conscription::TownDay> snapshots;
    for ( Castle * town : kingdom.GetCastles() ) {
        if ( !town->hasTrainingGrounds() ) continue;
        towns.push_back( town );
        snapshots.push_back( town->conscriptionDaySnapshot() );
    }
    int64_t gold = kingdom.GetFunds().gold;
    Conscription::scheduleDay( snapshots, gold, config, Monster( Monster::PEASANT ).GetCost().gold );
    kingdom.OddFundsResource( Funds( Resource::GOLD, static_cast<uint32_t>( kingdom.GetFunds().gold - gold ) ) );
    for ( size_t n = 0; n < towns.size(); ++n ) {
        Castle & town = *towns[n];
        const auto & snapshot = snapshots[n];
        uint32_t remaining = snapshot.fromGarrison;
        for ( size_t i = 0; i < town._army.Size() && remaining; ++i ) {
            Troop * troop = town._army.GetTroop( i );
            if ( troop->GetID() != Monster::PEASANT ) continue;
            const uint32_t taken = std::min( remaining, troop->GetCount() );
            remaining -= taken;
            if ( taken == troop->GetCount() ) troop->Reset();
            else troop->SetCount( troop->GetCount() - taken );
        }
        town._dwelling[0] -= snapshot.fromHut;
        town._conscription = snapshot.state;
        for ( size_t i = 0; i < 3; ++i ) town._dwelling[i + 1] = static_cast<uint32_t>( snapshot.pools[i] );
    }
}

Conscription::TownDay Castle::conscriptionDaySnapshot() const
{
    Conscription::TownDay result;
    result.state = _conscription;
    for ( size_t i = 0; i < _army.Size(); ++i ) {
        const auto * troop = _army.GetTroop( i );
        if ( troop->GetID() == Monster::PEASANT ) result.garrison += troop->GetCount();
    }
    result.hut = isBuild( DWELLING_MONSTER1 ) ? _dwelling[0] : 0;
    for ( size_t i = 0; i < 3; ++i ) result.pools[i] = _dwelling[i + 1];
    return result;
}

std::string Castle::dailyConscriptionDescription() const
{
    const auto & config = Conscription::settings();
    std::string description = "Trains up to " + std::to_string( config.dailyLimit ) + " Peasants per day into infantry. Conscripts are taken from the garrison first, at "
        + std::to_string( config.goldPerPeasant ) + " gold each. If needed, additional Peasants are conscripted directly from the Thatched Hut for "
        + std::to_string( config.goldPerPeasant + Monster( Monster::PEASANT ).GetCost().gold )
        + " gold each.\n\nTrained troops enter this town's recruitment pools and must be purchased normally.";
    std::vector<Conscription::TownDay> snapshots;
    size_t selected = 0;
    for ( const Castle * town : GetKingdom().GetCastles() ) {
        if ( !town->hasTrainingGrounds() ) continue;
        if ( town == this ) selected = snapshots.size();
        snapshots.push_back( town->conscriptionDaySnapshot() );
    }
    if ( snapshots.empty() ) return description;
    // Next week's special growth is not known yet. Do not present it as a firm forecast.
    if ( world.CountDay() % 7 == 0 )
        return description + "\n\nTomorrow's training depends on the new week's population changes.";
    int64_t gold = int64_t{ GetKingdom().GetFunds().gold } + GetKingdom().GetIncome( Kingdom::INCOME_ALL ).gold;
    const auto before = snapshots[selected].pools;
    Conscription::scheduleDay( snapshots, gold, config, Monster( Monster::PEASANT ).GetCost().gold );
    std::string forecast;
    const uint32_t dwellings[] = { DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
    for ( size_t i = 0; i < 3; ++i ) {
        const uint32_t count = static_cast<uint32_t>( snapshots[selected].pools[i] - before[i] );
        if ( !count ) continue;
        if ( !forecast.empty() ) forecast += ", ";
        forecast += std::to_string( count ) + " " + Monster( _race, GetActualDwelling( dwellings[i] ) ).GetPluralName( count );
    }
    return description + "\n\nExpected tomorrow: " + ( forecast.empty() ? "no troops" : forecast )
        + ".\nBased on current troops and funds, including daily income; events and intervening actions may change this.";
}
