#include "economy_military.h"
#include "economy_trace.h"
#include "army_troop.h"
#include "heroes.h"
#include "kingdom.h"
#include "taxpayer_test.h"

namespace
{
    void fill( Army & army, const AI::EconomyProject::Army & stacks )
    {
        for ( size_t i = 0; i < stacks.size(); ++i ) {
            if ( stacks[i].count ) army.GetTroop( i )->Set( Monster( stacks[i].monster ), stacks[i].count );
            else army.GetTroop( i )->Reset();
        }
    }
    int moraleBonus( const int race, const uint32_t mask )
    { return ( mask & BUILD_TAVERN ? 1 : 0 ) + ( race == Race::BARB && ( mask & BUILD_SPEC ) ? 2 : 0 ); }
    int luckBonus( const int race, const uint32_t mask )
    { return race == Race::SORC && ( mask & BUILD_SPEC ) ? 2 : 0; }
    int guildLevel( const uint32_t mask )
    {
        int level = 0;
        for ( uint32_t bit = BUILD_MAGEGUILD1; bit <= BUILD_MAGEGUILD5; bit <<= 1 ) { ++level; if ( !( mask & bit ) ) return level - 1; }
        return 5;
    }
}

AI::EconomyMilitary::Value AI::EconomyMilitary::localArmyValue( const Kingdom & kingdom, const P::Town & town, const Slot slot )
{
    const fheroes2::TaxpayerSimulationScope simulation;
    const auto found = std::find_if( kingdom.GetCastles().begin(), kingdom.GetCastles().end(), [&]( const Castle * c ) { return c && c->GetIndex() == town.tile; } );
    if ( found == kingdom.GetCastles().end() || ( *found )->GetColor() != kingdom.GetColor() ) return { false, 0, "town_context_changed" };
    const Castle & castle = **found;
    if ( castle.GetRace() != town.race ) return { false, 0, "town_race_changed" };
    const auto * hero = castle.GetHero();
    if ( ( hero ? hero->GetID() : -1 ) != town.hero || ( hero && hero->GetColor() != kingdom.GetColor() ) ) return { false, 0, "visitor_context_changed" };
    if ( slot == Slot::Visitor && !hero ) return { false, 0, "visitor_missing" };
    if ( slot != Slot::Visitor && slot != Slot::TownArmy ) return { false, 0, "unsupported_army_slot" };
    // Do not read spells stored for an unbuilt guild/library: these are hidden future information.
    if ( guildLevel( town.mask ) > castle.GetLevelMageGuild()
         || ( town.race == Race::WZRD && ( town.mask & BUILD_SPEC ) && !castle.isLibraryBuilt() ) )
        return { false, 0, "unknown_new_guild_spells" };

    std::optional<Captain> futureCaptain;
    const bool newCaptain = !castle.isBuild( BUILD_CAPTAIN ) && ( town.mask & BUILD_CAPTAIN );
    if ( newCaptain ) {
        futureCaptain.emplace( const_cast<Castle &>( castle ) ); // New local commander; the constructor only reads its home.
        futureCaptain->LoadDefaults( HeroBase::CAPTAIN, town.race );
        futureCaptain->SetSpellPoints( futureCaptain->GetMaxSpellPoints() );
        castle.GetMageGuild().trainHero( *futureCaptain, castle.GetLevelMageGuild(), castle.isLibraryBuilt() );
    }
    const HeroBase * commander = hero ? static_cast<const HeroBase *>( hero )
                                    : newCaptain ? static_cast<const HeroBase *>( &*futureCaptain ) : &castle.GetCaptain();
    Army hypothetical( const_cast<HeroBase *>( commander ) );
    hypothetical.SetColor( kingdom.GetColor() );
    if ( hero ) {
        fill( hypothetical, town.visitor );
        if ( slot == Slot::TownArmy ) {
            Army garrison;
            fill( garrison, town.garrison );
            hypothetical.ArrangeForCastleDefense( garrison ); // Same five-stack selection as the existing defense evaluator.
        }
    }
    else fill( hypothetical, town.garrison );

    uint32_t originalMask = 0;
    for ( uint32_t bit = 1; bit; bit <<= 1 ) if ( castle.isBuild( bit ) ) originalMask |= bit;
    const int castleMorale = moraleBonus( town.race, town.mask ) - moraleBonus( town.race, originalMask );
    const int castleLuck = luckBonus( town.race, town.mask ) - luckBonus( town.race, originalMask );
    const int armyMorale = hypothetical.GetMoraleModificator( nullptr );
    const int armyLuck = hypothetical.GetLuckModificator( nullptr );
    const int moraleDelta = armyMorale - commander->GetArmy().GetMoraleModificator( nullptr ) + castleMorale;
    const int luckDelta = armyLuck - commander->GetArmy().GetLuckModificator( nullptr ) + castleLuck;
    auto context = hypothetical.strengthContext();
    if ( hero ) {
        context.morale = hero->getMoraleWithModifiers( nullptr, moraleDelta );
        context.luck = hero->getLuckWithModifiers( nullptr, luckDelta );
    }
    else if ( town.mask & BUILD_CAPTAIN ) {
        const auto & captain = newCaptain ? *futureCaptain : castle.GetCaptain();
        context.morale = captain.getMoraleWithArmyDelta( moraleDelta );
        context.luck = captain.getLuckWithArmyDelta( luckDelta );
    }
    else { context.morale = armyMorale + castleMorale; context.luck = armyLuck + castleLuck; }
    // Apply the future Necromancer power bonus BEFORE clamping, as the normal commander getter does.
    const int powerDelta = town.race == Race::NECR ? ( town.mask & BUILD_SPEC ? 2 : 0 ) - ( originalMask & BUILD_SPEC ? 2 : 0 ) : 0;
    context.power = commander->powerWithCastleDelta( powerDelta );
    const double force = hypothetical.strengthWithContext( context );
    if ( !std::isfinite( force ) || force < 0 ) return { false, 0, "invalid_army_strength" };
    return { true, force, "local_force_current_mana_no_battle_or_fortification_forecast" };
}

AI::EconomyMilitary::Result AI::EconomyMilitary::evaluateObserved( Session & session, const G::Candidate & candidate, const int player, const Budget & budget )
{
    auto result = session.evaluate( candidate );
    if ( Settings::Get().taxpayerTestDiagnostics() >= 2 && fheroes2::taxpayerSimulationDepth == 0 ) {
        std::ostringstream gaps, utility;
        gaps << '[';
        bool first = true;
        for ( const auto & gap : result.gaps ) { if ( !first ) gaps << ','; first = false; gaps << Economy::jsonQuote( gap ); }
        gaps << ']';
        if ( result.utility ) utility << *result.utility; else utility << "null";
        const char * statuses[] = { "scored", "unfunded", "incomplete", "limit", "invalid" };
        ECONOMY_TRACE( 2, "MILITARY_EVALUATION", ",\"kind\":\"evaluation\",\"player\":" << player
                       << ",\"candidate\":" << candidate.id << ",\"status\":" << Economy::jsonQuote( statuses[static_cast<size_t>( result.status )] )
                       << ",\"funded\":" << ( result.financial.covered() ? "true" : "false" ) << ",\"utility\":" << utility.str()
                       << ",\"projection_budget_left\":" << budget.projections << ",\"force_budget_left\":" << budget.forces
                       << ",\"gaps\":" << gaps.str() << ",\"model\":\"dated_local_army_current_mana\"" );
        const auto evaluation = Economy::traceState().sequence;
        for ( const auto & day : result.days )
            ECONOMY_TRACE( 2, "MILITARY_DAY", ",\"kind\":\"evaluation\",\"player\":" << player << ",\"evaluation_seq\":" << evaluation
                           << ",\"candidate\":" << candidate.id << ",\"projected_day\":" << day.day << ",\"baseline\":" << day.baseline
                           << ",\"proposed\":" << day.proposed << ",\"difference\":" << day.difference );
    }
    return result;
}
