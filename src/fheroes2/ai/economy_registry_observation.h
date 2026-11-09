// C3D5C: snapshot-scoped owner of a hypothetical selected schedule, never live commitments.
#pragma once
#include "economy_registry.h"
#include "economy_stability.h"

namespace AI::EconomyRegistryObservation
{
    namespace V = EconomyRegistry;
    namespace C = EconomyCommitments;
    namespace G = EconomyCandidates;
    namespace P = EconomyProject;
    namespace X = EconomyMissions;
    namespace T = EconomyStability;
    enum class Status { Held, Empty, Staged, Unsupported, Limit };
    struct Link { uint64_t action = 0, sourceAction = 0, sourceProject = 0; size_t sourceIndex = 0; };
    struct Result
    {
        Status status = Status::Held;
        std::unique_ptr<V::Registry> registry;
        std::vector<Link> links;
        uint64_t selectedPlan = 0, token = 0;
        size_t programs = 0;
        std::string reason;
        bool actionable = false;
    };
    inline const char * name( const Status status )
    {
        switch ( status ) {
        case Status::Held: return "held";
        case Status::Empty: return "empty";
        case Status::Staged: return "staged";
        case Status::Unsupported: return "unsupported";
        case Status::Limit: return "limit";
        default: return "invalid";
        }
    }
    inline Result stage( const G::Context & context, const std::optional<X::Snapshot> & missions,
                         const T::Result & selected, const uint32_t player, const uint64_t token )
    {
        Result out;
        if ( !selected.certified || !selected.selected || *selected.selected >= selected.search.plans.size()
             || !selected.search.plans[*selected.selected].score.verified ) { out.reason = "registry_selection_not_verified"; return out; }
        if ( !missions || !missions->valid || !missions->gaps.empty() || !player || !token ) { out.reason = "registry_snapshot_or_identity_missing"; return out; }
        if ( !context.base.actions.empty() || !context.active.empty() ) {
            out.status = Status::Unsupported; out.reason = "live_commitment_ownership_not_connected"; return out;
        }
        const auto & candidate = selected.search.plans[*selected.selected].candidate;
        if ( candidate.actions.size() > 1024 ) { out.status = Status::Limit; out.reason = "registry_selected_schedule_limit"; return out; }
        P::Input input;
        if ( !G::materialize( context, candidate, input ) ) { out.reason = "registry_selected_identity_invalid"; return out; }
        struct Source { P::Action action; size_t index; };
        std::vector<Source> source;
        for ( size_t i = 0; i < input.actions.size(); ++i ) source.push_back( { input.actions[i], i } );
        std::stable_sort( source.begin(), source.end(), []( const auto & a, const auto & b ) { return a.action.day < b.action.day; } );
        std::vector<C::Program> drafts; std::map<int32_t, size_t> byTown;
        for ( const auto & step : source ) {
            if ( step.action.kind != P::Kind::Build && step.action.kind != P::Kind::Recruit ) {
                out.status = Status::Unsupported; out.reason = "registry_selected_transfer_or_delivery_unsupported"; return out;
            }
            if ( !byTown.count( step.action.town ) ) {
                byTown[step.action.town] = drafts.size(); C::Program draft; draft.town = step.action.town; drafts.push_back( draft );
            }
            auto action = step.action; action.id = action.project = 0;
            drafts[byTown.at( action.town )].pending.push_back( action );
        }
        if ( drafts.size() > 64 ) { out.status = Status::Limit; out.reason = "registry_selected_program_limit"; return out; }
        // These programs share the selected plan's funds. Do not assume independent
        // abandonment or invent cross-town dependencies: all stay protected.
        auto registry = std::make_unique<V::Registry>( player, token );
        const auto refreshed = registry->refresh( context, *missions );
        if ( refreshed.status != V::Status::Ready ) { out.reason = refreshed.reason; return out; }
        for ( auto & draft : drafts ) {
            const auto accepted = registry->stage( std::move( draft ) );
            if ( accepted.status != V::Status::Ready ) { out.reason = accepted.reason; return out; }
        }
        const auto imported = registry->view();
        if ( !imported.context || !imported.permissions.empty() ) { out.reason = "registry_protected_import_failed"; return out; }
        auto registered = imported.context->base.actions;
        std::stable_sort( registered.begin(), registered.end(), []( const auto & a, const auto & b ) { return a.day < b.day; } );
        if ( registered.size() != source.size() ) { out.reason = "registry_schedule_size_changed"; return out; }
        std::vector<Link> links;
        for ( size_t i = 0; i < source.size(); ++i ) {
            // Only identities change. Dates, quantities, required flags and exact
            // same-day operation order must survive grouping by local beneficiary.
            if ( !G::sameAction( source[i].action, registered[i] ) ) {
                out.status = Status::Unsupported; out.reason = "registry_grouping_would_reorder_schedule"; return out;
            }
            links.push_back( { registered[i].id, source[i].action.id, source[i].action.project, source[i].index } );
        }
        out.status = registered.empty() ? Status::Empty : Status::Staged;
        out.reason = "hypothetical_snapshot_only_never_applied"; out.registry = std::move( registry ); out.links = std::move( links );
        out.selectedPlan = candidate.id; out.token = token; out.programs = drafts.size(); return out;
    }
    void log( const Result &, int player );
}
