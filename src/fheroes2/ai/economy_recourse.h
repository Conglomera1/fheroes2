// C3D1: prepare a feasible common reference; observation only, no live releases.
#pragma once
#include "economy_reserve.h"

namespace AI::EconomyRecourse
{
    namespace G = EconomyCandidates;
    namespace P = EconomyProject;
    namespace M = EconomyMilitary;
    namespace D = EconomyDefense;
    namespace R = EconomyRisk;
    // Caller identifies a genuinely revisable, not-yet-executed project. Full
    // snapshots prevent stale authorization from releasing a different schedule.
    struct Revisable { uint64_t project = 0; std::vector<P::Action> actions; };
    struct Failure { uint64_t scenario, action; uint32_t day; std::string reason; };
    struct Release { uint64_t project; std::vector<uint64_t> actions; std::vector<Failure> evidence; };
    enum class Status { Ready, Blocked, Incomplete, Limit, Invalid };
    struct Result
    {
        Status status = Status::Invalid;
        std::unique_ptr<R::Session> session; // Present ONLY for the new checked common reference.
        std::vector<Release> released;
        std::vector<Failure> failures;
        EconomyReserve::Assessment funding;
        size_t attempts = 0;
        bool regenerate = false, actionable = false;
        std::string reason;
    };
    inline Result prepare( const G::Context & original, const std::vector<Revisable> & permissions,
                           const std::vector<M::Assignment> & assignments, const M::Scorer & scorer, M::Budget & budget,
                           const D::Registry & defenses, const D::Scorer & defender, const R::Registry & risks )
    {
        Result out;
        if ( original.base.actions.size() > 1024 || permissions.size() > 64 ) { out.reason = "recourse_input_limit"; return out; }
        std::map<uint64_t, const P::Action *> actions;
        for ( const auto & a : original.base.actions ) {
            if ( !a.id || !a.project || a.day < original.base.start.economy.day || !actions.emplace( a.id, &a ).second ) {
                out.reason = "invalid_retained_actions"; return out;
            }
        }
        std::set<uint64_t> authorized;
        size_t entries = original.base.actions.size();
        for ( const auto & permission : permissions ) {
            if ( !permission.project || permission.actions.empty() || permission.actions.size() > 2048 - entries
                 || !authorized.insert( permission.project ).second ) { out.reason = "invalid_release_permission"; return out; }
            entries += permission.actions.size(); std::set<uint64_t> ids;
            for ( const auto & a : permission.actions ) {
                const auto found = actions.find( a.id );
                if ( found == actions.end() || a.project != permission.project || found->second->project != a.project
                     || !ids.insert( a.id ).second || !G::sameAction( a, *found->second ) ) { out.reason = "stale_release_permission"; return out; }
            }
            for ( const auto & a : original.base.actions ) if ( a.project == permission.project && !ids.count( a.id ) ) {
                out.reason = "partial_project_permission"; return out;
            }
        }
        auto working = original;
        // Each unsuccessful retry must remove at least one explicitly authorized project.
        for ( size_t pass = 0; pass <= permissions.size(); ++pass ) {
            auto session = std::make_unique<R::Session>( working, assignments, scorer, budget, defenses, defender, risks );
            if ( !session->context() ) { out.reason = "invalid_risk_context"; return out; }
            if ( budget.projections < session->requiredProjections() ) {
                out.status = Status::Limit; out.reason = "shared_projection_preflight_limit"; return out;
            }
            G::Candidate reference; reference.id = uint64_t{ 1 } << 62;
            auto checked = session->evaluate( reference ); ++out.attempts;
            out.funding = EconomyReserve::assess( checked );
            if ( out.funding.protectedTrajectory ) {
                out.session = std::move( session ); out.status = Status::Ready;
                out.reason = out.released.empty() ? "original_reference_verified" : "revised_reference_verified"; return out;
            }
            if ( checked.status == R::Status::Limit ) { out.status = Status::Limit; out.reason = "shared_work_limit"; return out; }
            if ( checked.status == R::Status::Invalid ) { out.reason = "invalid_reference_context"; return out; }
            std::map<uint64_t, std::vector<Failure>> failing;
            for ( const auto & variant : checked.variants ) {
                const auto & f = variant.evaluation.financial;
                const bool material = f.status == P::Status::Unfunded && f.reason == "payment_unavailable";
                const bool illegal = f.status == P::Status::Invalid && ( f.reason == "building_unavailable" || f.reason == "recruit_stock_unavailable"
                    || f.reason == "army_slots_unavailable" || f.reason == "visitor_unavailable" || f.reason == "peasant_transfer_unavailable" );
                auto record = [&]( const uint64_t id, const std::string & reason, const uint32_t day ) {
                    const auto a = actions.find( id );
                    if ( a == actions.end() ) return;
                    Failure evidence{ variant.scenario, id, day, reason };
                    out.failures.push_back( evidence ); failing[a->second->project].push_back( std::move( evidence ) );
                };
                if ( material || illegal ) record( f.failedAction, f.reason, f.failedDay );
                if ( f.covered() ) for ( const auto id : f.cancelledRequiredActions ) {
                    const auto a = actions.find( id );
                    if ( a != actions.end() ) record( id, "required_destination_lost", a->second->day );
                }
            }
            // A protected commitment that fails remains a blocker: never silently erase it.
            if ( std::any_of( failing.begin(), failing.end(), [&]( const auto & p ) { return !authorized.count( p.first ); } ) ) {
                out.status = Status::Blocked; out.reason = "failed_project_not_revisable"; return out;
            }
            if ( failing.empty() ) { out.status = Status::Incomplete; out.reason = "no_material_release_evidence"; return out; }
            for ( const auto & item : failing ) {
                Release release; release.project = item.first; release.evidence = item.second;
                for ( const auto & a : working.base.actions ) if ( a.project == item.first ) release.actions.push_back( a.id );
                working.base.actions.erase( std::remove_if( working.base.actions.begin(), working.base.actions.end(),
                    [&]( const auto & a ) { return a.project == item.first; } ), working.base.actions.end() );
                out.released.push_back( std::move( release ) ); authorized.erase( item.first );
            }
            // Old optional continuations may depend on a released prerequisite. Regenerate
            // proposals for this frame instead of resurrecting an old plan.
            working.active.clear(); out.regenerate = true;
        }
        out.status = Status::Incomplete; out.reason = "recourse_pass_limit"; return out;
    }
    Result observe( const G::Context &, const std::vector<Revisable> &, const std::vector<M::Assignment> &, const M::Scorer &, M::Budget &,
                    const D::Registry &, const D::Scorer &, const R::Registry &, int player );
}
