// Bounded proposals, not funded/selected game decisions. C2 validates materialized plans.
#pragma once
#include "economy_project.h"
#include <cmath>
#include <functional>
#include <sstream>
#include <tuple>

namespace AI::EconomyCandidates
{
    namespace P = EconomyProject;
    enum class Family : size_t { Recruit, ArmyUpgrade, Production, Fiscal, Training, SupplyWait, Count };
    constexpr size_t familyCount = static_cast<size_t>( Family::Count );
    constexpr uint32_t bit( const Family family ) { return uint32_t{ 1 } << static_cast<size_t>( family ); }
    struct Candidate
    {
        uint64_t id = 0;
        int32_t town = -1;
        uint32_t families = 0;
        std::vector<P::Action> actions;
        double quickGain = 0; // Presort only, never military utility or proof of arrival.
        uint64_t taxOpportunity = 0, nominalGold = 0;
        bool urgent = false;
        bool milestone = false;
        uint32_t deadline = 0, priority = 0;
        std::string label;
    };
    struct Request // Quantities/deadlines supplied by a caller with actual military knowledge.
    {
        int32_t town = -1;
        size_t pool = 0;
        uint32_t count = 0, day = 0, priority = 0;
        bool urgent = false;
    };
    struct Context
    {
        P::Input base; // base.actions are retained commitments, not charged twice.
        std::vector<P::Action> active; // Optional continuation reference, not forced into every alternative.
        std::vector<Request> requests;
    };
    struct Budget { size_t proposals = 16384; }; // Share across repeated generation in the same turn.
    struct Counts { size_t generated = 0, duplicates = 0, localDropped = 0, commonDropped = 0, kept = 0; };
    struct Result
    {
        bool valid = false;
        std::vector<Candidate> references, projects;
        std::array<Counts, familyCount> families{};
        std::set<std::string> gaps;
        std::map<std::string, size_t> blocked;
        size_t urgentDropped = 0, townsVisited = 0;
        bool longerHorizonNeeded = false;
    };
    struct Observation { Context context; Result proposals; };
    Observation observe( const Kingdom & kingdom, uint32_t throughDay, Budget & budget );

    inline bool sameAction( const P::Action & a, const P::Action & b )
    { return a.day == b.day && a.town == b.town && a.kind == b.kind && a.building == b.building && a.pool == b.pool && a.count == b.count && a.required == b.required; }
    inline std::string signature( const Candidate & candidate )
    {
        std::ostringstream out;
        for ( const auto & a : candidate.actions ) out << a.day << ':' << a.town << ':' << static_cast<int>( a.kind ) << ':' << a.building << ':' << a.pool << ':' << a.count << ':' << a.required << ';';
        return out.str();
    }
    inline bool materialize( const Context & context, const Candidate & candidate, P::Input & output )
    {
        output = context.base;
        std::map<uint64_t, P::Action> ids;
        uint64_t next = 0;
        for ( const auto & action : output.actions ) {
            if ( !action.id || !action.project || action.id >= ( uint64_t{ 1 } << 63 ) || !ids.emplace( action.id, action ).second ) return false;
            next = std::max( next, action.id );
        }
        for ( const auto & action : candidate.actions ) if ( action.id ) next = std::max( next, action.id );
        for ( auto action : candidate.actions ) {
            if ( action.id && ids.count( action.id ) ) {
                if ( !sameAction( action, ids.at( action.id ) ) ) return false;
                continue; // The active reference may name the same retained commitment.
            }
            if ( !action.id ) {
                if ( next >= ( uint64_t{ 1 } << 63 ) - 1 ) return false;
                action.id = ++next; action.project = candidate.id;
            }
            if ( !action.project || action.id >= ( uint64_t{ 1 } << 63 ) ) return false;
            ids.emplace( action.id, action ); output.actions.push_back( action );
        }
        return true;
    }
    inline P::Action recruit( const int32_t town, const size_t pool, const uint32_t count, const uint32_t day )
    { P::Action a; a.kind = P::Kind::Recruit; a.town = town; a.pool = pool; a.count = count; a.day = day; return a; }
    inline uint64_t capacity( const P::Army & army, const int monster )
    {
        for ( const auto & s : army ) if ( s.count && s.monster == monster ) return std::numeric_limits<uint32_t>::max() - s.count;
        for ( const auto & s : army ) if ( !s.count ) return std::numeric_limits<uint32_t>::max();
        return 0;
    }

    inline Result generate( const Context & context, Budget & budget )
    {
        Result result;
        // Honest coverage: these must be completed before any claim of exhausted opportunities.
        result.gaps = { "financial_and_military_evaluation_pending", "combinations_pending", "army_upgrades_unsupported",
                        "market_and_remote_transport_unsupported", "special_weeks_unmodelled", "quantity_and_calendar_sampling" };
        const auto & base = context.base;
        const uint32_t today = base.start.economy.day;
        if ( !today || base.throughDay < today || uint64_t{ base.throughDay } - today > 28
             || base.start.towns.size() != base.start.economy.towns.size() || base.start.towns.size() > 256 || context.requests.size() > 1024
             || !EconomyBudget::validAmounts( P::funds( base.start.economy ) ) ) { result.gaps.insert( "invalid_input" ); return result; }
        std::map<int32_t, size_t> townIndex;
        for ( size_t i = 0; i < base.start.towns.size(); ++i ) {
            const auto & cycle = base.start.economy.towns[i].troops.state;
            uint64_t remaining = 0;
            for ( size_t k = 0; k < 3; ++k ) if ( cycle.used[k] <= cycle.quotas[k] ) remaining += cycle.quotas[k] - cycle.used[k];
            if ( cycle.next >= 3 || cycle.lossRemainder >= Conscription::successScale || !remaining
                 || !std::equal( cycle.used.begin(), cycle.used.end(), cycle.quotas.begin(), []( const uint32_t used, const uint32_t quota ) { return used <= quota; } ) ) {
                result.gaps.insert( "invalid_training_state" ); return result;
            }
            if ( !townIndex.emplace( base.start.towns[i].tile, i ).second || base.start.towns[i].tile < 0
                 || base.start.towns[i].tile != base.start.economy.towns[i].tile || base.start.towns[i].catalog.size() > 32 ) {
                result.gaps.insert( "invalid_town" ); return result;
            }
        }
        uint64_t serial = 1;
        Candidate reference; reference.id = serial++; reference.label = "no_new_investment";
        P::Input checked;
        if ( !materialize( context, reference, checked ) ) { result.gaps.insert( "invalid_commitments" ); return result; }
        result.references.push_back( reference );
        if ( !context.active.empty() ) {
            reference.id = serial++; reference.label = "active_continuation"; reference.actions = context.active;
            if ( !materialize( context, reference, checked ) ) { result.gaps.insert( "invalid_active_reference" ); return result; }
            result.references.push_back( reference );
        }
        result.gaps.insert( "best_ordinary_reference_not_ranked" );
        std::vector<Candidate> protectedProjects;
        auto requests = context.requests;
        std::stable_sort( requests.begin(), requests.end(), []( const Request & a, const Request & b ) {
            return std::tie( a.day, a.priority, a.town, a.pool, a.count ) < std::tie( b.day, b.priority, b.town, b.pool, b.count );
        } );
        for ( const auto & request : requests ) {
            if ( !townIndex.count( request.town ) || request.pool >= 6 || !request.count || request.day < today ) {
                result.gaps.insert( "invalid_quantity_request" ); return result;
            }
            Candidate c; c.id = serial++; c.town = request.town; c.families = bit( request.day == today ? Family::Recruit : Family::SupplyWait );
            c.actions = { recruit( request.town, request.pool, request.count, request.day ) };
            c.actions[0].required = request.urgent; c.urgent = request.urgent; c.deadline = request.day; c.priority = request.priority;
            c.label = "supplied_quantity_deadline";
            const auto & town = base.start.towns[townIndex.at( request.town )];
            const auto & unit = town.dwellings[request.pool].creature( town.mask );
            const double quick = unit.quickStrength * request.count;
            if ( std::isfinite( quick ) && quick >= 0 ) c.quickGain = quick;
            else result.gaps.insert( "missing_quick_strength" );
            if ( unit.price[0] >= 0 && unit.price[0] <= EconomyBudget::resourceMaximum ) c.nominalGold = static_cast<uint64_t>( unit.price[0] ) * request.count;
            protectedProjects.push_back( std::move( c ) );
        }
        if ( requests.empty() ) result.gaps.insert( "military_quantity_deadlines_not_supplied" );
        std::vector<int32_t> order;
        for ( const auto & item : townIndex ) order.push_back( item.first );
        if ( !order.empty() ) std::rotate( order.begin(), order.begin() + today % order.size(), order.end() );
        std::vector<Candidate> local;
        std::set<std::string> selectedSignatures;
        std::set<std::string> urgentSeen;
        // Urgent requests precede optional local cuts; deduplication preserves their priority.
        for ( const auto & c : protectedProjects ) if ( c.urgent ) {
            auto & stats = result.families[static_cast<size_t>( c.deadline == today ? Family::Recruit : Family::SupplyWait )];
            ++stats.generated;
            if ( c.deadline > base.throughDay ) { result.longerHorizonNeeded = true; result.gaps.insert( "effect_or_action_beyond_horizon" ); }
            if ( !urgentSeen.insert( signature( c ) ).second ) { ++stats.duplicates; continue; }
            if ( result.projects.size() >= 64 ) { ++result.urgentDropped; ++stats.commonDropped; result.gaps.insert( "urgent_selection_limit" ); continue; }
            selectedSignatures.insert( signature( c ) ); result.projects.push_back( c );
        }
        auto strength = [&]( const P::Creature & unit, const uint32_t quantity ) {
            if ( !std::isfinite( unit.quickStrength ) || unit.quickStrength < 0 ) { result.gaps.insert( "missing_quick_strength" ); return 0.0; }
            const double value = unit.quickStrength * quantity;
            if ( !std::isfinite( value ) ) { result.gaps.insert( "quick_strength_overflow" ); return 0.0; }
            return value;
        };
        // Select promising external destinations without converting gold into military points.
        struct Target { int32_t town; size_t pool; double value; };
        std::vector<Target> targets;
        for ( const auto tile : order ) {
            const auto & town = base.start.towns[townIndex.at( tile )];
            for ( size_t pool = 0; pool < 6; ++pool ) if ( town.mask & town.dwellings[pool].base ) {
                const auto & unit = town.dwellings[pool].creature( town.mask );
                if ( unit.id && ( capacity( town.garrison, unit.id ) || ( town.hero >= 0 && capacity( town.visitor, unit.id ) ) ) )
                    targets.push_back( { tile, pool, strength( unit, 1 ) } );
            }
        }
        std::stable_sort( targets.begin(), targets.end(), []( const Target & a, const Target & b ) { return a.value > b.value; } );
        if ( targets.size() > 2 ) { targets.resize( 2 ); result.gaps.insert( "fiscal_destination_reduction" ); }
        for ( const auto tile : order ) {
            if ( !budget.proposals ) { result.gaps.insert( "proposal_limit" ); break; }
            ++result.townsVisited;
            const size_t index = townIndex.at( tile );
            const auto & town = base.start.towns[index];
            const auto & economic = base.start.economy.towns[index];
            std::vector<Candidate> raw;
            std::map<std::string, size_t> seen;
            auto emit = [&]( Candidate c, const Family family ) {
                if ( !budget.proposals ) { result.gaps.insert( "proposal_limit" ); return; }
                --budget.proposals;
                auto & stats = result.families[static_cast<size_t>( family )]; ++stats.generated;
                c.families |= bit( family ); c.town = tile;
                const auto key = signature( c );
                const auto existing = seen.find( key );
                if ( existing != seen.end() ) {
                    raw[existing->second].families |= c.families;
                    raw[existing->second].taxOpportunity = std::max( raw[existing->second].taxOpportunity, c.taxOpportunity );
                    raw[existing->second].milestone = raw[existing->second].milestone || c.milestone;
                    if ( c.milestone ) raw[existing->second].label = c.label;
                    ++stats.duplicates; return;
                }
                if ( raw.size() >= 96 ) { result.gaps.insert( "local_proposal_limit" ); ++stats.localDropped; return; }
                c.id = serial++;
                for ( const auto & a : c.actions ) if ( a.day > base.throughDay ) { result.longerHorizonNeeded = true; result.gaps.insert( "effect_or_action_beyond_horizon" ); }
                seen.emplace( key, raw.size() ); raw.push_back( std::move( c ) );
            };
            auto chain = [&]( const uint32_t goal, const uint32_t notBefore, Candidate & c ) {
                uint32_t mask = town.mask;
                uint64_t date = std::max<uint64_t>( notBefore, uint64_t{ today } + ( town.buildAvailable ? 0 : 1 ) );
                std::set<uint32_t> visiting;
                std::function<bool( uint32_t )> add = [&]( const uint32_t id ) {
                    if ( mask & id ) return true;
                    if ( !id || ( id & ( id - 1 ) ) || !visiting.insert( id ).second ) { result.gaps.insert( "cyclic_or_invalid_prerequisite" ); return false; }
                    const auto found = town.catalog.find( id );
                    if ( found == town.catalog.end() ) { result.gaps.insert( "unsupported_prerequisite" ); return false; }
                    if ( ( town.disabled & id ) || ( town.race == Race::KNGT && id == BUILD_TRAINING_GROUNDS
                         && ( !base.config.enabled || ( town.disabled & BUILD_WEL2 ) ) ) ) { ++result.blocked["disabled_building"]; return false; }
                    const uint32_t required = town.race == Race::KNGT && id == BUILD_TRAINING_GROUNDS ? base.config.requiredBuildings : found->second.prerequisites;
                    // Castle construction is not silently skipped when C2 cannot model it.
                    if ( !( mask & BUILD_CASTLE ) && !add( BUILD_CASTLE ) ) return false;
                    for ( uint32_t prerequisite = 1; prerequisite; prerequisite <<= 1 )
                        if ( ( required & prerequisite ) && !add( prerequisite ) ) return false;
                    while ( date <= std::numeric_limits<uint32_t>::max() && std::any_of( base.actions.begin(), base.actions.end(), [&]( const P::Action & a ) {
                        return a.town == tile && a.kind == P::Kind::Build && a.day == date;
                    } ) ) ++date;
                    if ( date > std::numeric_limits<uint32_t>::max() ) { result.gaps.insert( "calendar_overflow" ); return false; }
                    P::Action a; a.kind = P::Kind::Build; a.town = tile; a.day = static_cast<uint32_t>( date++ ); a.building = id;
                    c.actions.push_back( a );
                    int64_t gold = found->second.price[0];
                    if ( town.race == Race::KNGT && id == BUILD_TRAINING_GROUNDS ) gold = base.config.trainingGold;
                    else if ( town.race == Race::KNGT && id == BUILD_WEL2 && ( mask & BUILD_TRAINING_GROUNDS ) ) gold = base.config.farmGold;
                    if ( gold < 0 ) { result.gaps.insert( "invalid_cost" ); return false; }
                    c.nominalGold += static_cast<uint64_t>( gold );
                    if ( town.race == Race::KNGT && id == BUILD_TRAINING_GROUNDS ) mask &= ~BUILD_WEL2;
                    else if ( town.race == Race::KNGT && id == BUILD_WEL2 ) mask &= ~BUILD_TRAINING_GROUNDS;
                    mask |= id; visiting.erase( id ); return true;
                };
                return add( goal );
            };
            auto addPurchase = [&]( Candidate & c, const int32_t destination, const size_t pool, const uint32_t quantity, const uint32_t day ) {
                const auto & target = base.start.towns[townIndex.at( destination )];
                uint32_t mask = target.mask;
                for ( const auto & a : c.actions ) if ( a.town == destination && a.kind == P::Kind::Build ) mask |= a.building;
                const auto & unit = target.dwellings[pool].creature( mask );
                c.actions.push_back( recruit( destination, pool, quantity, day ) );
                c.quickGain = strength( unit, quantity );
                if ( unit.price[0] >= 0 ) c.nominalGold += static_cast<uint64_t>( unit.price[0] ) * quantity;
            };
            auto futureDay = [&]( const uint64_t day, uint32_t & resultDay ) {
                if ( day > std::numeric_limits<uint32_t>::max() ) { result.gaps.insert( "calendar_overflow" ); return false; }
                resultDay = static_cast<uint32_t>( day ); return true;
            };
            auto firstSuccess = [&]( const uint64_t firstMorning, uint32_t & when, size_t & pool ) {
                if ( !base.config.enabled || !base.config.successRate ) return false;
                const uint32_t rate = std::min( base.config.successRate, Conscription::successScale );
                uint64_t needed = economic.troops.state.lossRemainder / rate + 1;
                uint64_t date = firstMorning;
                const uint64_t boundary = std::min<uint64_t>( std::numeric_limits<uint32_t>::max(), uint64_t{ today } + 28 );
                for ( ; date <= boundary; ++date ) {
                    const uint32_t limit = Conscription::dailyLimitForDay( base.config, static_cast<uint32_t>( date ) );
                    if ( needed <= limit ) break;
                    needed -= limit;
                }
                if ( date > boundary || economic.troops.state.total == std::numeric_limits<uint64_t>::max() ) {
                    result.gaps.insert( "training_effect_beyond_detailed_window" ); result.longerHorizonNeeded = true; return false;
                }
                auto state = economic.troops.state;
                const auto output = Conscription::advance( state, 1, base.config.quotas );
                pool = 0; while ( pool < 3 && !output[pool] ) ++pool;
                if ( pool >= 3 ) { result.gaps.insert( "unknown_training_composition" ); return false; }
                ++pool; when = static_cast<uint32_t>( date ); return true;
            };
            for ( size_t pool = 0; pool < 6; ++pool ) {
                const auto & dwelling = town.dwellings[pool];
                const auto & unit = dwelling.creature( town.mask );
                if ( !unit.id ) continue;
                if ( !EconomyBudget::validAmounts( unit.price ) ) { result.gaps.insert( "invalid_creature_price" ); continue; }
                if ( town.mask & dwelling.base ) {
                    uint64_t maximum = pool == 0 ? economic.troops.hut : pool < 4 ? economic.troops.pools[pool - 1] : town.extraPools[pool - 4];
                    maximum = std::min<uint64_t>( maximum, std::numeric_limits<uint32_t>::max() );
                    maximum = std::min( maximum, std::max( capacity( town.garrison, unit.id ), town.hero >= 0 ? capacity( town.visitor, unit.id ) : uint64_t{ 0 } ) );
                    const auto cash = P::funds( base.start.economy );
                    for ( size_t r = 0; r < 7; ++r ) if ( unit.price[r] ) maximum = std::min<uint64_t>( maximum, static_cast<uint64_t>( cash[r] / unit.price[r] ) );
                    for ( const auto quantity : { uint32_t{ 1 }, static_cast<uint32_t>( maximum ) } ) if ( quantity && quantity <= maximum ) {
                        Candidate c; c.label = "immediate_recruit"; addPurchase( c, tile, pool, quantity, today ); emit( std::move( c ), Family::Recruit );
                    }
                    const uint64_t weekly = uint64_t{ today } + 8 - ( ( today - 1 ) % 7 + 1 );
                    for ( const uint64_t date : { uint64_t{ today } + 1, weekly, uint64_t{ base.throughDay } } ) if ( date > today ) {
                        uint32_t day; if ( !futureDay( date, day ) ) continue;
                        Candidate c; c.label = "known_weekly_or_horizon_wait"; addPurchase( c, tile, pool, 1, day ); emit( std::move( c ), Family::SupplyWait );
                    }
                }
                const uint32_t goal = !( town.mask & dwelling.base ) ? dwelling.base : dwelling.secondUpgrade && !( town.mask & dwelling.secondUpgrade ) ? dwelling.secondUpgrade
                    : dwelling.upgrade && !( town.mask & dwelling.upgrade ) ? dwelling.upgrade : 0;
                if ( goal ) for ( const uint32_t start : { today, base.throughDay } ) {
                    Candidate c; c.label = "building_chain_then_recruit";
                    if ( !chain( goal, start, c ) || c.actions.empty() ) continue;
                    addPurchase( c, tile, pool, 1, c.actions.back().day ); emit( std::move( c ), Family::Production );
                }
            }
            if ( town.race == Race::KNGT && base.start.economy.taxRate ) {
                if ( !( town.mask & BUILD_WEL2 ) ) for ( const uint32_t start : { today, base.throughDay > today ? base.throughDay - 1 : today } ) {
                    Candidate prefix; prefix.label = "farm_finances_recruitment";
                    if ( !chain( BUILD_WEL2, start, prefix ) || prefix.actions.empty() ) continue;
                    const uint64_t firstTax = uint64_t{ prefix.actions.back().day } + 1;
                    uint32_t day; if ( !futureDay( firstTax, day ) ) continue;
                    const uint64_t cap = EconomyBudget::resourceMaximum;
                    const uint64_t peasants = std::min( cap, std::min( cap, economic.troops.garrison ) + std::min( cap, economic.visitingPeasants ) );
                    prefix.taxOpportunity = base.start.economy.requireFarms ? std::min( cap, peasants * base.start.economy.taxRate ) : 0;
                    for ( const auto & target : targets ) {
                        Candidate c = prefix; addPurchase( c, target.town, target.pool, 1, day ); emit( std::move( c ), Family::Fiscal );
                    }
                }
                else if ( economic.troops.hut ) {
                    Candidate c; c.label = "recruit_taxpayer";
                    c.taxOpportunity = base.start.economy.taxRate;
                    addPurchase( c, tile, 0, 1, today ); emit( std::move( c ), Family::Fiscal );
                }
            }
            if ( town.race == Race::KNGT && !( town.mask & BUILD_TRAINING_GROUNDS ) && base.config.enabled && base.config.successRate ) {
                for ( const uint32_t start : { today, base.throughDay > today ? base.throughDay - 1 : today } ) {
                    Candidate c; c.label = "training_chain_then_first_success";
                    if ( !chain( BUILD_TRAINING_GROUNDS, start, c ) || c.actions.empty() ) continue;
                    uint32_t when; size_t pool;
                    if ( !firstSuccess( uint64_t{ c.actions.back().day } + 1, when, pool ) ) continue;
                    addPurchase( c, tile, pool, 1, when ); emit( std::move( c ), Family::Training );
                }
            }
            if ( town.race == Race::KNGT && ( !base.config.enabled || !base.config.successRate ) )
                ++result.blocked[base.config.enabled ? "zero_training_success" : "training_disabled"];
            if ( town.race == Race::KNGT && ( town.mask & BUILD_TRAINING_GROUNDS ) ) {
                uint32_t when; size_t pool;
                if ( firstSuccess( uint64_t{ today } + 1, when, pool ) ) {
                    Candidate c; c.label = "existing_training_next_success"; c.milestone = true;
                    addPurchase( c, tile, pool, 1, when ); emit( std::move( c ), Family::SupplyWait );
                }
            }
            for ( const auto & c : protectedProjects ) if ( !c.urgent && c.town == tile ) emit( c, c.deadline == today ? Family::Recruit : Family::SupplyWait );
            if ( town.hero >= 0 ) {
                const uint64_t people = P::population( town.visitor );
                const uint64_t deposit = std::min( { P::count( town.visitor ), people ? people - 1 : 0, capacity( town.garrison, Monster::PEASANT ) } );
                const uint64_t withdrawal = std::min( P::count( town.garrison ), capacity( town.visitor, Monster::PEASANT ) );
                for ( const auto kind : { P::Kind::ToGarrison, P::Kind::ToVisitor } ) {
                    const auto quantity = static_cast<uint32_t>( kind == P::Kind::ToGarrison ? deposit : withdrawal );
                    if ( !quantity ) continue;
                    Candidate c; c.label = kind == P::Kind::ToGarrison ? "local_peasant_deposit" : "local_peasant_withdrawal";
                    P::Action a; a.kind = kind; a.town = tile; a.day = today; a.count = quantity;
                    c.actions = { a }; emit( std::move( c ), Family::SupplyWait );
                }
            }
            auto better = []( const Candidate & a, const Candidate & b ) {
                if ( a.taxOpportunity != b.taxOpportunity ) return a.taxOpportunity > b.taxOpportunity;
                if ( a.quickGain > b.quickGain ) return true;
                if ( a.quickGain < b.quickGain ) return false;
                if ( a.nominalGold != b.nominalGold ) return a.nominalGold < b.nominalGold;
                return a.id < b.id;
            };
            std::stable_sort( raw.begin(), raw.end(), better );
            std::set<uint64_t> kept;
            for ( size_t family = 0; family < familyCount; ++family ) {
                std::vector<const Candidate *> eligible;
                for ( const auto & c : raw ) if ( c.families & ( uint32_t{ 1 } << family ) ) eligible.push_back( &c );
                if ( eligible.empty() ) continue;
                std::stable_sort( eligible.begin(), eligible.end(), [&]( const Candidate * a, const Candidate * b ) {
                    if ( family == static_cast<size_t>( Family::Fiscal ) && a->taxOpportunity != b->taxOpportunity ) return a->taxOpportunity > b->taxOpportunity;
                    if ( a->quickGain > b->quickGain ) return true;
                    if ( a->quickGain < b->quickGain ) return false;
                    if ( a->nominalGold != b->nominalGold ) return a->nominalGold < b->nominalGold;
                    return a->id < b->id;
                } );
                kept.insert( eligible[0]->id );
                if ( eligible.size() > 1 ) {
                    const auto milestone = std::find_if( eligible.begin() + 1, eligible.end(), []( const Candidate * c ) { return c->milestone; } );
                    if ( milestone != eligible.end() ) kept.insert( ( *milestone )->id );
                    else {
                        const auto cheap = std::min_element( eligible.begin() + 1, eligible.end(), []( const Candidate * a, const Candidate * b ) {
                            return a->nominalGold != b->nominalGold ? a->nominalGold < b->nominalGold : a->id > b->id;
                        } );
                        kept.insert( ( *cheap )->id );
                    }
                }
            }
            // Reuse spare family places for action kinds otherwise absent, without inventing military gain for a transfer.
            std::set<P::Kind> represented;
            for ( const auto & c : raw ) if ( kept.count( c.id ) ) for ( const auto & a : c.actions ) represented.insert( a.kind );
            for ( const auto & c : raw ) if ( kept.size() < 12 ) {
                if ( std::any_of( c.actions.begin(), c.actions.end(), [&]( const P::Action & a ) { return !represented.count( a.kind ); } ) ) {
                    kept.insert( c.id ); for ( const auto & a : c.actions ) represented.insert( a.kind );
                }
            }
            for ( const auto & c : raw ) if ( kept.size() < 12 ) kept.insert( c.id );
            for ( auto & c : raw ) {
                if ( kept.count( c.id ) ) local.push_back( std::move( c ) );
                else {
                    result.gaps.insert( "local_selection_reduction" );
                    for ( size_t f = 0; f < familyCount; ++f ) if ( c.families & ( uint32_t{ 1 } << f ) ) ++result.families[f].localDropped;
                }
            }
        }
        // Up to four new representatives per family, alternating towns; then fill by rounds.
        std::array<size_t, familyCount> cursors{};
        std::array<std::map<int32_t, size_t>, familyCount> positions;
        auto take = [&]( const size_t family ) {
            for ( size_t visit = 0; visit < order.size(); ++visit ) {
                const auto tile = order[cursors[family]++ % order.size()];
                auto & position = positions[family][tile];
                for ( ; position < local.size(); ++position ) {
                    const auto & c = local[position];
                    if ( c.town != tile || !( c.families & ( uint32_t{ 1 } << family ) ) || selectedSignatures.count( signature( c ) ) ) continue;
                    selectedSignatures.insert( signature( c ) ); result.projects.push_back( c ); ++position; return true;
                }
            }
            return false;
        };
        for ( size_t f = 0; f < familyCount; ++f ) for ( size_t n = 0; n < 4 && result.projects.size() < 64; ++n ) if ( !take( f ) ) break;
        bool progress = true;
        while ( progress && result.projects.size() < 64 ) {
            progress = false;
            for ( size_t f = 0; f < familyCount && result.projects.size() < 64; ++f ) progress = take( f ) || progress;
        }
        for ( const auto & c : local ) if ( !selectedSignatures.count( signature( c ) ) ) {
            result.gaps.insert( "common_selection_reduction" );
            for ( size_t f = 0; f < familyCount; ++f ) if ( c.families & ( uint32_t{ 1 } << f ) ) ++result.families[f].commonDropped;
        }
        for ( const auto & c : result.projects ) for ( size_t f = 0; f < familyCount; ++f ) if ( c.families & ( uint32_t{ 1 } << f ) ) ++result.families[f].kept;
        result.valid = true;
        return result;
    }
}
