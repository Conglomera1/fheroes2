// C3D5A: import explicit pending local programs into copied contexts only.
#pragma once
#include "economy_missions.h"
#include "economy_recourse.h"

namespace AI::EconomyCommitments
{
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    namespace Q = EconomyRecourse;
    namespace X = EconomyMissions;
    // Caller-issued identity of ONE frozen decision snapshot, including its effective INI.
    // The adapter cannot detect a live world/config change unless the caller changes revision.
    struct Stamp { uint32_t player = 0, day = 0; uint64_t revision = 0; };
    enum class Source { ExplicitProgram, LegacyHint };
    enum class Destination { Town, StationaryVisitor, RemoteHero };
    struct Program
    {
        uint64_t id = 0;
        int32_t town = -1, hero = -1;
        Source source = Source::ExplicitProgram;
        Destination destination = Destination::Town;
        std::vector<P::Action> pending; // Complete remaining schedule, never already paid steps.
        std::set<uint64_t> dependsOn;
        bool allowAbandon = false, independent = false, executedPrefix = false;
    };
    struct Book { Stamp stamp; std::vector<Program> programs; };
    enum class Status { Ready, Invalid, Unsupported, Limit };
    struct Result
    {
        Status status = Status::Invalid;
        std::optional<G::Context> context;
        std::vector<Q::Revisable> permissions;
        std::set<uint64_t> protectedProjects, beyondHorizon;
        std::string reason;
        bool actionable = false, fundingVerified = false;
    };
    inline bool sameStamp( const Stamp & a, const Stamp & b )
    { return a.player == b.player && a.day == b.day && a.revision == b.revision; }
    inline bool same( const P::Action & a, const P::Action & b )
    { return a.id == b.id && a.project == b.project && G::sameAction( a, b ); }

    inline Result import( const G::Context & original, const X::Snapshot & missions, const Stamp & current, const Book & book )
    {
        Result out;
        const auto today = original.base.start.economy.day;
        if ( !current.player || !current.revision || !today || current.day != today || !sameStamp( book.stamp, current )
             || !missions.valid || missions.day != today || original.base.throughDay < today ) {
            out.reason = "stale_or_invalid_commitment_snapshot"; return out;
        }
        if ( book.programs.size() > 64 || original.base.actions.size() > 1024 || original.base.start.towns.size() > 256
             || original.active.size() > 1024 || original.requests.size() > 1024 || missions.heroes.size() > 256 ) {
            out.status = Status::Limit; out.reason = "commitment_input_limit"; return out;
        }
        std::map<int32_t, const P::Town *> towns;
        for ( const auto & town : original.base.start.towns ) if ( town.tile < 0 || !town.available || !towns.emplace( town.tile, &town ).second ) {
            out.reason = "invalid_commitment_towns"; return out;
        }
        std::map<uint64_t, P::Action> retained;
        std::map<uint64_t, std::set<uint64_t>> projectActions;
        for ( const auto & action : original.base.actions ) {
            if ( !action.id || action.id >= ( uint64_t{1} << 63 ) || !action.project || action.day < today
                 || !retained.emplace( action.id, action ).second ) { out.reason = "invalid_existing_commitments"; return out; }
            projectActions[action.project].insert( action.id );
        }
        std::map<uint64_t, const Program *> programs;
        std::map<uint64_t, std::set<uint64_t>> pendingIds;
        size_t entries = 0;
        // Validate everything before publishing a context or any release permission.
        for ( const auto & program : book.programs ) {
            if ( !program.id || !programs.emplace( program.id, &program ).second || !towns.count( program.town ) || program.pending.empty() ) {
                out.reason = "invalid_commitment_program"; return out;
            }
            if ( program.pending.size() > 1024 - entries || program.dependsOn.size() > 64 ) {
                out.status = Status::Limit; out.reason = "commitment_schedule_limit"; return out;
            }
            entries += program.pending.size();
            if ( program.source != Source::ExplicitProgram ) { out.reason = "legacy_hint_is_not_commitment_authority"; return out; }
            if ( program.destination == Destination::RemoteHero ) {
                out.status = Status::Unsupported; out.reason = "remote_delivery_not_implemented"; return out;
            }
            if ( program.destination != Destination::Town && program.destination != Destination::StationaryVisitor ) {
                out.reason = "invalid_commitment_destination"; return out;
            }
            const auto & town = *towns.at( program.town );
            if ( program.destination == Destination::Town && program.hero != -1 ) { out.reason = "ambiguous_town_beneficiary"; return out; }
            if ( program.destination == Destination::StationaryVisitor ) {
                const auto hero = std::find_if( missions.heroes.begin(), missions.heroes.end(), [&]( const auto & h ) { return h.id == program.hero; } );
                if ( program.hero < 0 || town.hero != program.hero || !missions.stationaryVisitors.count( program.hero ) || hero == missions.heroes.end()
                     || hero->town != program.town || hero->tile != program.town || hero->use != X::Use::FixedLocal
                     || !hero->patrol || hero->patrolRadius != 0 ) {
                    out.status = Status::Unsupported; out.reason = "beneficiary_local_assignment_unverified"; return out;
                }
                // Recruit auto-placement can prefer the garrison. No exact visitor delivery is implemented.
                out.status = Status::Unsupported; out.reason = "exact_visitor_delivery_not_implemented"; return out;
            }
            uint32_t previousDay = today;
            for ( const auto & action : program.pending ) {
                if ( !action.id || action.id >= ( uint64_t{1} << 63 ) || action.project != program.id || action.town != program.town
                     || action.day < previousDay || !pendingIds[program.id].insert( action.id ).second ) {
                    out.reason = "invalid_or_unordered_pending_action"; return out;
                }
                previousDay = action.day;
                if ( action.kind != P::Kind::Build && action.kind != P::Kind::Recruit ) {
                    out.status = Status::Unsupported; out.reason = "nonlocal_or_transfer_commitment_unsupported"; return out;
                }
                if ( ( action.kind == P::Kind::Build && ( !action.building || ( action.building & ( action.building - 1 ) ) || action.count || action.pool ) )
                     || ( action.kind == P::Kind::Recruit && ( action.building || action.pool >= 6 || !action.count ) ) ) {
                    out.reason = "invalid_commitment_action_shape"; return out;
                }
                const auto old = retained.find( action.id );
                if ( old != retained.end() && !same( old->second, action ) ) { out.reason = "stale_or_conflicting_action_identity"; return out; }
            }
            const auto existing = projectActions.find( program.id );
            if ( existing != projectActions.end() && existing->second != pendingIds.at( program.id ) ) {
                out.reason = "partial_or_extended_existing_program"; return out;
            }
        }
        // Dependencies must be explicit and refer to this complete book. Protect any
        // prerequisite from isolated recourse; dependent projects may still be revisable.
        std::set<uint64_t> needed;
        std::map<uint64_t, size_t> indegree;
        for ( const auto & item : programs ) {
            indegree[item.first] = item.second->dependsOn.size();
            for ( const auto dependency : item.second->dependsOn ) {
                if ( !programs.count( dependency ) || dependency == item.first ) { out.reason = "unknown_or_self_dependency"; return out; }
                needed.insert( dependency );
            }
        }
        std::set<uint64_t> ready;
        for ( const auto & item : indegree ) if ( !item.second ) ready.insert( item.first );
        std::vector<uint64_t> order;
        while ( !ready.empty() ) {
            const auto id = *ready.begin(); ready.erase( ready.begin() ); order.push_back( id );
            for ( const auto & item : programs ) if ( item.second->dependsOn.count( id ) && --indegree.at( item.first ) == 0 ) ready.insert( item.first );
        }
        if ( order.size() != programs.size() ) { out.reason = "cyclic_commitment_dependencies"; return out; }
        auto working = original;
        std::set<uint64_t> ids;
        for ( const auto & action : original.base.actions ) ids.insert( action.id );
        for ( const auto id : order ) {
            const auto & program = *programs.at( id );
            for ( const auto & action : program.pending ) {
                const auto old = retained.find( action.id );
                if ( old == retained.end() ) {
                    if ( !ids.insert( action.id ).second ) { out.reason = "duplicate_new_action_identity"; return out; }
                    working.base.actions.push_back( action );
                }
                if ( action.day > original.base.throughDay ) out.beyondHorizon.insert( id );
            }
        }
        if ( working.base.actions.size() > 1024 ) { out.status = Status::Limit; out.reason = "merged_commitment_limit"; return out; }
        for ( const auto & active : original.active ) if ( active.id ) {
            const auto commitment = std::find_if( working.base.actions.begin(), working.base.actions.end(), [&]( const auto & a ) { return a.id == active.id; } );
            if ( commitment != working.base.actions.end() && !same( active, *commitment ) ) {
                out.reason = "conflicting_active_commitment_identity"; return out;
            }
        }
        // Existing order (especially same-day actions) is preserved. Dependencies that
        // demand a later prerequisite or different same-day order are not repaired here.
        std::map<uint64_t, size_t> first, last;
        std::map<uint64_t, uint32_t> firstDay, lastDay;
        for ( size_t i = 0; i < working.base.actions.size(); ++i ) {
            const auto & action = working.base.actions[i];
            if ( !first.count( action.project ) ) { first[action.project] = i; firstDay[action.project] = action.day; }
            last[action.project] = i;
            firstDay[action.project] = std::min( firstDay.at( action.project ), action.day );
            lastDay[action.project] = std::max( lastDay[action.project], action.day );
        }
        for ( const auto & item : programs ) for ( const auto dependency : item.second->dependsOn )
            if ( lastDay.at( dependency ) > firstDay.at( item.first )
                 || ( lastDay.at( dependency ) == firstDay.at( item.first ) && last.at( dependency ) >= first.at( item.first ) ) ) {
                out.status = Status::Unsupported; out.reason = "dependency_schedule_requires_rescheduling"; return out;
            }
        // Permissions describe complete programs exactly. They are drafts for Q, not
        // permission to mutate the live registry. No funding, stock or legality claim.
        const bool completeExisting = std::all_of( projectActions.begin(), projectActions.end(), [&]( const auto & item ) { return programs.count( item.first ); } );
        for ( const auto & item : programs ) {
            const auto & program = *item.second;
            if ( completeExisting && program.allowAbandon && program.independent && !program.executedPrefix && !needed.count( program.id ) )
                out.permissions.push_back( { program.id, program.pending } );
            else out.protectedProjects.insert( program.id );
        }
        for ( const auto & item : projectActions ) if ( !programs.count( item.first ) ) out.protectedProjects.insert( item.first );
        out.context = std::move( working ); out.status = Status::Ready; out.reason = "pending_local_programs_imported_not_funded";
        return out;
    }
}
