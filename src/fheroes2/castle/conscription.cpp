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
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "army_troop.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "payment.h"
#include "resource.h"
#include "system.h"
#include "tinyconfig.h"
#include "translations.h"
#include "tools.h"
#include "world.h"
#include "economy_trace.h"

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
        std::string comments;
        const auto section = [&]( const char * title ) {
            comments += std::string( "\n# " ) + title + '\n';
        };
        const auto read = [&]( const char * key, const int32_t fallback, const int32_t maximum ) {
            if ( config.Exists( key ) ) {
                comments.clear();
                return std::clamp( config.IntParams( key ), 0, maximum );
            }
            if ( additions ) {
                additions << comments;
                additions << key << " = " << fallback << '\n';
            }
            comments.clear();
            return fallback;
        };
        section( "Conscription" );
        section( "Conscription mode: 0 = disabled, 1 = fixed daily quotas, 2 = orders per town." );
        const int32_t legacyMode = config.Exists( "conscription.enabled" ) ? std::clamp( config.IntParams( "conscription.enabled" ), 0, 1 ) : 1;
        result.mode = read( "conscription.mode", legacyMode, 2 );
        result.enabled = result.mode != 0;
        section( "Gold per garrison Peasant, including failures. Hut conscripts also pay the normal Peasant price." );
        result.goldPerPeasant = read( "conscription.gold_per_peasant", 50, 1000000 );
        section( "Mode 1: Daily conscription limit per town. Odd days of the week: 1, 3, 5, 7. Even days: 2, 4, 6." );
        result.oddDayLimit = read( "conscription.odd_day_limit", 2, 10000 );
        result.evenDayLimit = read( "conscription.even_day_limit", 3, 10000 );
        result.dailyLimit = std::max( result.oddDayLimit, result.evenDayLimit );
        const auto readDecimal = [&]( const char * key, const double minimum, const double maximum ) {
            double parsed = 1.0;
            if ( config.Exists( key ) ) {
                const auto text = config.StrParams( key );
                try {
                    size_t consumed = 0;
                    const double decimal = std::stod( text, &consumed );
                    if ( consumed == text.size() && std::isfinite( decimal ) ) parsed = std::clamp( decimal, minimum, maximum );
                }
                catch ( const std::exception & ) {
                    // Invalid values keep the default.
                }
            }
            else if ( additions ) additions << comments << key << " = 1.0\n";
            comments.clear();
            return parsed;
        };
        section( "Mode 2: increase per tier as a fraction of the base fee. 1.0 = the same amount; 0 = flat pricing." );
        comments += "# If the base fee is zero, this is an increment in gold.\n";
        result.goldIncrement = readDecimal( "conscription.gold_increment", 0.0, 1000000.0 );
        section( "Mode 2: multiply each successive increment by this factor. Minimum 1.0." );
        result.goldIncrementMultiplier = readDecimal( "conscription.gold_increment_multiplier", 1.0, 1000000.0 );
        section( "Mode 2: number of Peasants per price tier. Minimum 1." );
        result.goldIncrementStep = std::max( 1, read( "conscription.gold_increment_step", 1, std::numeric_limits<int32_t>::max() ) );
        section( "Mode 2: 0 = individual price per rank, 1 = last rank's price for the entire order." );
        result.uniformBatchPrice = read( "conscription.uniform_batch_price", 0, 1 ) != 0;
        section( "Proportion of conscripted Peasants who complete their training." );
        comments += "# 1.0 = all, 0.9 = 90%. Failed conscripts are lost and still paid for.\n";
        if ( config.Exists( "conscription.peasant_success_ratio" ) ) {
            const std::string text = config.StrParams( "conscription.peasant_success_ratio" );
            try {
                size_t consumed = 0;
                const double ratio = std::stod( text, &consumed );
                if ( consumed == text.size() && std::isfinite( ratio ) )
                    result.successRate = static_cast<uint32_t>( std::llround( std::clamp( ratio, 0.0, 1.0 ) * successScale ) );
            }
            catch ( const std::exception & ) {
                // Invalid values keep the default 100% success rate.
            }
        }
        else if ( additions ) additions << comments << "conscription.peasant_success_ratio = 1.0\n";
        comments.clear();
        section( "Troops per full cycle of successful training. Each town keeps its own progress." );
        result.quotas = { static_cast<uint32_t>( read( "conscription.archers", 8, 10000 ) ),
                          static_cast<uint32_t>( read( "conscription.pikemen", 5, 10000 ) ),
                          static_cast<uint32_t>( read( "conscription.swordsmen", 4, 10000 ) ) };
        if ( result.quotas == Quotas{} ) result.quotas = { 8, 5, 4 };
        section( "Farm to Training Grounds" );
        result.trainingGold = read( "training_grounds.gold", 1750, 1000000 );
        result.trainingWood = read( "training_grounds.wood", 10, 1000000 );
        result.trainingOre = read( "training_grounds.ore", 5, 1000000 );
        section( "Training Grounds back to Farm" );
        result.farmGold = read( "farm_reconversion.gold", 1250, 1000000 );
        result.farmWood = read( "farm_reconversion.wood", 0, 1000000 );
        result.farmOre = read( "farm_reconversion.ore", 0, 1000000 );
        section( "Training Grounds requirements" );
        comments += "# The Farm, Archery Range, Blacksmith and Armory are always required.\n";
        result.requiredBuildings = BUILD_WEL2 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4;
        const auto require = [&]( const char * key, const BuildingType building, const int fallback = 0 ) {
            if ( read( key, fallback, 1 ) ) result.requiredBuildings |= building;
        };
        section( "Command. 0 = no, 1 = yes" );
        require( "training_grounds.require_captains_quarters", BUILD_CAPTAIN, 1 );
        section( "Infantry upgrades. 0 = no, 1 = yes" );
        require( "training_grounds.require_upgraded_archery_range", DWELLING_UPGRADE2 );
        require( "training_grounds.require_upgraded_blacksmith", DWELLING_UPGRADE3 );
        require( "training_grounds.require_upgraded_armory", DWELLING_UPGRADE4 );
        section( "Cavalry and Paladins. 0 = no, 1 = yes (base or upgraded), 2 = upgraded only" );
        const int cavalry = read( "training_grounds.require_jousting_arena", 0, 2 );
        if ( cavalry ) result.requiredBuildings |= cavalry == 2 ? DWELLING_UPGRADE5 : DWELLING_MONSTER5;
        const int cathedral = read( "training_grounds.require_cathedral", 0, 2 );
        if ( cathedral ) result.requiredBuildings |= cathedral == 2 ? DWELLING_UPGRADE6 : DWELLING_MONSTER6;
        section( "Castle defenses. 0 = no, 1 = yes" );
        require( "training_grounds.require_moat", BUILD_MOAT );
        require( "training_grounds.require_left_turret", BUILD_LEFTTURRET );
        require( "training_grounds.require_right_turret", BUILD_RIGHTTURRET );
        require( "training_grounds.require_fortifications", BUILD_SPEC );
        section( "Non-military buildings" );
        section( "Minimum Mage Guild level required: 1 to 5. Set to 0 to disable." );
        const int mageGuild = read( "training_grounds.require_mage_guild", 0, 5 );
        if ( mageGuild ) result.requiredBuildings |= BUILD_MAGEGUILD1 << ( mageGuild - 1 );
        section( "Other buildings. 0 = no, 1 = yes" );
        require( "training_grounds.require_well", BUILD_WELL );
        require( "training_grounds.require_tavern", BUILD_TAVERN );
        require( "training_grounds.require_statue", BUILD_STATUE );
        require( "training_grounds.require_thieves_guild", BUILD_THIEVESGUILD );
        require( "training_grounds.require_marketplace", BUILD_MARKETPLACE );
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
    const Economy::CauseScope cause( "conscription", static_cast<int>( kingdom.GetColor() ) );
    const auto & config = Conscription::settings();
    std::vector<Castle *> towns;
    std::vector<Conscription::TownDay> snapshots;
    for ( Castle * town : kingdom.GetCastles() ) {
        if ( !town->hasTrainingGrounds() ) continue;
        towns.push_back( town );
        snapshots.push_back( town->conscriptionDaySnapshot() );
    }
    int64_t gold = kingdom.GetFunds().gold;
    const auto before = snapshots;
    const uint32_t peasantCost = Monster( Monster::PEASANT ).GetCost().gold;
    Conscription::scheduleDay( snapshots, gold, config, peasantCost, world.CountDay() );
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
        if ( config.mode == 2 ) {
            const uint32_t stockLimited = static_cast<uint32_t>( std::min<uint64_t>( snapshot.order, before[n].garrison + before[n].hut ) );
            const uint32_t attempts = snapshot.fromGarrison + snapshot.fromHut;
            [[maybe_unused]] const bool goldLimited = Conscription::batchCost( config, stockLimited, before[n].garrison, peasantCost )
                                                    > static_cast<uint64_t>( std::max<int64_t>( 0, snapshot.goldBefore ) );
            ECONOMY_TRACE( 1, "CONSCRIPTION_ORDER_EXECUTION", ",\"kind\":\"actual\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                << ",\"town\":" << town.GetIndex() << ",\"requested\":" << snapshot.order << ",\"attempted\":" << attempts
                << ",\"gold_before\":" << snapshot.goldBefore << ",\"stock_limited\":" << ( stockLimited < snapshot.order ? "true" : "false" )
                << ",\"gold_limited\":" << ( goldLimited ? "true" : "false" )
                << ",\"pool_or_counter_limited\":" << ( !goldLimited && attempts < stockLimited ? "true" : "false" )
                << ",\"training_fees\":" << snapshot.trainingFees << ",\"hut_purchase\":" << uint64_t{ snapshot.fromHut } * peasantCost
                << ",\"quoted_training_fees\":" << Conscription::trainingCost( config, snapshot.order )
                << ",\"uniform_batch_price\":" << ( config.uniformBatchPrice ? "true" : "false" )
                << ",\"price_increment\":" << config.goldIncrement << ",\"increment_multiplier\":" << config.goldIncrementMultiplier
                << ",\"increment_step\":" << config.goldIncrementStep );
        }
        ECONOMY_TRACE( 1, "CONSCRIPTION_DAY", ",\"kind\":\"actual\",\"player\":" << static_cast<int>( kingdom.GetColor() )
                       << ",\"town\":" << town.GetIndex() << ",\"order\":" << n << ",\"limit\":" << ( config.mode == 2 ? snapshot.order : Conscription::dailyLimitForDay( config, world.CountDay() ) )
                       << ",\"garrison_used\":" << snapshot.fromGarrison << ",\"hut_used\":" << snapshot.fromHut << ",\"failed\":" << snapshot.failed
                       << ",\"garrison_before\":" << before[n].garrison << ",\"garrison_after\":" << snapshot.garrison
                       << ",\"hut_before\":" << before[n].hut << ",\"hut_after\":" << snapshot.hut
                       << ",\"enabled\":" << ( config.enabled ? "true" : "false" ) << ",\"odd_limit\":" << config.oddDayLimit << ",\"even_limit\":" << config.evenDayLimit
                       << ",\"success_rate_scaled\":" << config.successRate << ",\"fee_per_attempt\":" << config.goldPerPeasant
                       << ",\"fees\":" << snapshot.trainingFees + uint64_t{ snapshot.fromHut } * peasantCost
                       << ",\"recruits\":[" << snapshot.pools[0] - before[n].pools[0] << ',' << snapshot.pools[1] - before[n].pools[1] << ',' << snapshot.pools[2] - before[n].pools[2]
                       << "],\"success_total_before\":" << before[n].state.total << ",\"success_total_after\":" << snapshot.state.total
                       << ",\"loss_remainder_before\":" << before[n].state.lossRemainder << ",\"loss_remainder_after\":" << snapshot.state.lossRemainder );
    }
}

Conscription::TownDay Castle::conscriptionDaySnapshot() const
{
    Conscription::TownDay result;
    result.state = _conscription;
    result.order = _conscriptionOrder;
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
    if ( !config.enabled ) return _( "Conscription is disabled in knight_mod.ini." );
    if ( config.mode == 2 ) {
        std::string hutFee = _( "Conscripts are taken from the garrison first. Those taken from the Thatched Hut cost an extra %{price} gold each." );
        StringReplace( hutFee, "%{price}", Monster( Monster::PEASANT ).GetCost().gold );
        return std::string( _( "Set how many Peasants to train as infantry each day." ) ) + "\n\n" + hutFee + "\n\n"
               + _( "Trained troops are added to this town's recruitment pools and must still be purchased at the usual price." );
    }
    std::string description = _( "Automatically trains up to %{count} Peasants per day into infantry." );
    StringReplace( description, "%{count}", std::max( config.oddDayLimit, config.evenDayLimit ) );
    description += "\n\n";
    description += _( "Conscripts are taken from the garrison first: %{fee} gold each. Peasants taken from the Thatched Hut also cost %{price} gold to recruit." );
    StringReplace( description, "%{fee}", config.goldPerPeasant );
    StringReplace( description, "%{price}", Monster( Monster::PEASANT ).GetCost().gold );
    description += "\n\n";
    description += _( "Trained troops enter this town's recruitment pools and must be purchased normally." );
    if ( config.successRate < Conscription::successScale ) {
        std::ostringstream percentage;
        percentage.precision( 8 );
        percentage << 100.0 * config.successRate / Conscription::successScale;
        std::string success = _( "Training success: %{percent}%. Failed conscripts are lost and still paid for." );
        StringReplace( success, "%{percent}", percentage.str() );
        description += "\n\n" + success;
    }
    std::vector<Conscription::TownDay> snapshots;
    size_t selected = 0;
    for ( const Castle * town : GetKingdom().GetCastles() ) {
        if ( !town->hasTrainingGrounds() ) continue;
        if ( town == this ) selected = snapshots.size();
        snapshots.push_back( town->conscriptionDaySnapshot() );
        if ( world.CountDay() % 7 == 0 && town->isBuild( DWELLING_MONSTER1 ) ) {
            // Forecast normal weekly growth; exceptional weeks may change the actual result.
            snapshots.back().hut += Monster( Monster::PEASANT ).GetGrown()
                + ( town->isBuild( BUILD_WELL ) ? GetGrownWell() : 0 );
        }
    }
    if ( !config.enabled || snapshots.empty() ) return description;
    int64_t gold = int64_t{ GetKingdom().GetFunds().gold } + GetKingdom().GetIncome( Kingdom::INCOME_ALL ).gold;
    const auto before = snapshots[selected].pools;
    Conscription::scheduleDay( snapshots, gold, config, Monster( Monster::PEASANT ).GetCost().gold, world.CountDay() + 1 );
    std::string forecast;
    const uint32_t dwellings[] = { DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
    for ( size_t i = 0; i < 3; ++i ) {
        const uint32_t count = static_cast<uint32_t>( snapshots[selected].pools[i] - before[i] );
        if ( !count ) continue;
        if ( !forecast.empty() ) forecast += "\n";
        forecast += std::to_string( count ) + " " + Monster( _race, GetActualDwelling( dwellings[i] ) ).GetPluralName( count );
    }
    return description + "\n\n" + _( "Expected Tomorrow:" ) + "\n" + ( forecast.empty() ? _( "No troops" ) : forecast );
}

std::string Castle::conscriptionOrderPreview( const uint32_t count ) const
{
    const auto & config = Conscription::settings();
    auto state = _conscription;
    const auto recruits = Conscription::train( state, count, config );
    const uint32_t dwellings[] = { DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4 };
    std::string forecast;
    for ( size_t i = 0; i < recruits.size(); ++i ) {
        if ( !recruits[i] ) continue;
        forecast += std::to_string( recruits[i] ) + " " + Monster( _race, GetActualDwelling( dwellings[i] ) ).GetPluralName( static_cast<uint32_t>( recruits[i] ) )
                    + "\n";
    }
    if ( forecast.empty() ) forecast = std::string( _( "No troops" ) ) + "\n";
    const auto cost = Conscription::trainingCost( config, count );
    std::string price = cost == Conscription::feeDisplayLimit ? _( "Training cost: over %{gold} gold" ) : _( "Training cost: %{gold} gold" );
    StringReplace( price, "%{gold}", std::to_string( cost ) );
    return forecast + "\n" + price;
}
