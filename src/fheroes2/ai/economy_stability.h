// C3C2: read-only stability certificates; never a live execution authorization.
#pragma once
#include "economy_search.h"

namespace AI::EconomyStability
{
    namespace S = EconomySearch;
    namespace R = EconomyRisk;
    namespace M = EconomyMilitary;
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    struct Decision { bool accepted = false; std::string reason; double gain = 0, margin = 0; };
    // Only fully checked plans in the SAME frozen comparison frame may cross the gate.
    inline Decision compare( const S::Score & next, const S::Score & old )
    {
        Decision out;
        if ( !next.verified || !old.verified || !next.defenseComplete || !old.defenseComplete || !next.scope || next.scope != old.scope || next.defenses.size() != old.defenses.size()
             || !next.robust || !old.robust || !next.central || !old.central
             || !std::isfinite( *next.robust ) || !std::isfinite( *old.robust ) || !std::isfinite( *next.central ) || !std::isfinite( *old.central ) ) {
            out.reason = "comparison_unverified"; return out;
        }
        for ( size_t i = 0; i < next.defenses.size(); ++i ) if ( next.defenses[i] != old.defenses[i] ) {
            out.accepted = next.defenses[i]; out.reason = out.accepted ? "urgent_defense_made_viable" : "priority_defense_degraded"; return out;
        }
        out.gain = *next.robust - *old.robust;
        out.margin = 0.10 * std::max( { 1.0, std::abs( *old.robust ), std::abs( *next.robust ) } );
        if ( S::utilityOrder( *next.central, *old.central ) < 0 ) { out.reason = "central_utility_degraded"; return out; }
        out.accepted = out.gain > out.margin;
        out.reason = out.accepted ? "robust_margin_passed" : "robust_margin_not_passed";
        return out;
    }
    // Return optional role transitions only; retained base commitments are common, not new decisions.
    inline std::vector<size_t> changes( const G::Context & context, const G::Candidate & candidate )
    {
        std::map<int32_t, std::pair<int, uint32_t>> roles;
        for ( const auto & town : context.base.start.towns ) roles[town.tile] = { town.race, town.mask };
        struct Step { const P::Action * action; std::optional<size_t> index; };
        std::vector<Step> steps;
        for ( const auto & a : context.base.actions ) steps.push_back( { &a, {} } );
        for ( size_t i = 0; i < candidate.actions.size(); ++i ) steps.push_back( { &candidate.actions[i], i } );
        std::stable_sort( steps.begin(), steps.end(), []( const auto & a, const auto & b ) { return a.action->day < b.action->day; } );
        std::vector<size_t> out;
        for ( const auto & step : steps ) {
            const auto & a = *step.action;
            auto found = roles.find( a.town );
            if ( a.kind != P::Kind::Build || found == roles.end() || found->second.first != Race::KNGT ) continue;
            auto & mask = found->second.second;
            if ( a.building == BUILD_TRAINING_GROUNDS || ( a.building == BUILD_WEL2 && ( mask & BUILD_TRAINING_GROUNDS ) ) ) {
                if ( step.index ) out.push_back( *step.index );
                mask &= ~( BUILD_WEL2 | BUILD_TRAINING_GROUNDS );
            }
            mask |= a.building;
        }
        return out;
    }
    inline S::Score compact( const R::Result & evaluation )
    {
        S::Score score; score.reserve = EconomyReserve::assess( evaluation ); score.risk = evaluation.status; score.gaps = evaluation.gaps; score.robust = evaluation.robustUtility;
        score.variants = evaluation.variants.size();
        if ( evaluation.variants.empty() ) return score;
        const auto & central = evaluation.variants[0].evaluation;
        score.central = central.utility; score.centralFunded = central.financial.covered(); score.scope = central.defense.scope;
        score.defenseComplete = central.defense.complete();
        for ( const auto & city : central.defense.cities ) {
            score.defenseComplete = score.defenseComplete && city.viable.has_value(); score.defenses.push_back( city.viable.value_or( false ) );
        }
        score.verified = score.reserve.protectedTrajectory && evaluation.status == R::Status::Scored && score.centralFunded && score.defenseComplete && score.scope
            && score.robust && score.central && std::isfinite( *score.robust ) && std::isfinite( *score.central );
        for ( const auto & variant : evaluation.variants ) {
            const auto & f = variant.evaluation.financial;
            score.witnesses.push_back( { variant.scenario, f.covered(), f.failedDay, f.failedAction, f.reason,
                                        f.cancelledActions.size(), f.cancelledRequiredActions.size(), variant.minimumFunds } );
        }
        return score;
    }
    struct Repair { uint32_t day; int32_t town; size_t pool; uint32_t requested, kept; };
    struct Cached { S::Score score; std::vector<Repair> repairs; };
    struct Proof { std::vector<Repair> repairs; size_t plan = 0, action = 0; int32_t town = 0; uint32_t day = 0, building = 0; Decision decision; S::Score without; bool cached = false; };
    struct Rejection { size_t plan; std::string reason; };
    struct Result
    {
        S::Result search;
        std::optional<size_t> selected, fallback, current;
        std::vector<Proof> proofs;
        std::vector<Rejection> rejected;
        size_t reserved = 0, evaluations = 0, cacheHits = 0;
        std::set<std::string> gaps;
        bool certified = false, actionable = false;
    };
    inline bool demonstrablyImpossible( const S::Plan & plan )
    {
        // Missing information, lost optional steps and budget limits are NOT evidence of impossibility.
        if ( plan.score.verified ) return false;
        return std::any_of( plan.score.witnesses.begin(), plan.score.witnesses.end(), []( const auto & witness ) {
            return !witness.funded && ( witness.reason == "payment_unavailable" || witness.reason == "building_unavailable"
                || witness.reason == "recruit_stock_unavailable" || witness.reason == "visitor_unavailable" || witness.reason == "peasant_transfer_unavailable" );
        } );
    }
    inline Result select( R::Session & evaluator, const G::Result & proposals, M::Budget & budget, S::Work & work )
    {
        Result result;
        // Retain up to one quarter for certificates, within the SAME budget; no counters are restored.
        result.reserved = std::min<size_t>( 64, std::min<size_t>( budget.projections, 256 ) / 4 );
        result.search = S::search( evaluator, proposals, budget, work, result.reserved );
        result.gaps = result.search.gaps;
        const auto * context = evaluator.context(); if ( !context ) return result;
        auto & plans = result.search.plans;
        for ( size_t i = 0; i < plans.size(); ++i ) if ( plans[i].score.verified && changes( *context, plans[i].candidate ).empty()
             && ( !result.fallback || S::prefer( plans[i], plans[*result.fallback] ) ) ) result.fallback = i;
        result.current = result.search.active ? result.search.active : result.search.noInvestment;
        // Preserve the stronger verified no-conversion alternative as the real opportunity cost.
        std::optional<size_t> reference = result.current;
        if ( reference && !plans[*reference].score.verified ) reference.reset();
        if ( result.fallback && ( !reference || S::prefer( plans[*result.fallback], plans[*reference] ) ) ) reference = result.fallback;
        std::map<std::string, Cached> cache;
        for ( const auto & plan : plans ) {
            cache.emplace( S::scheduleKey( plan.candidate ), Cached{ plan.score, {} } );
        }
        std::vector<size_t> remaining;
        for ( size_t i = 0; i < plans.size(); ++i ) if ( plans[i].score.verified ) remaining.push_back( i );
        while ( !remaining.empty() ) {
            auto best = remaining.begin();
            for ( auto it = remaining.begin(); it != remaining.end(); ++it ) if ( S::prefer( plans[*it], plans[*best] ) ) best = it;
            const size_t index = *best; remaining.erase( best ); const auto & plan = plans[index];
            const auto conversions = changes( *context, plan.candidate );
            if ( conversions.empty() ) { result.selected = index; result.certified = true; break; }
            if ( result.search.longerHorizonNeeded ) { result.rejected.push_back( { index, "common_horizon_review_required" } ); continue; }
            if ( !reference ) { result.rejected.push_back( { index, "current_reference_unverified" } ); continue; }
            // An invalid active commitment may be abandoned, but cannot waive the per-conversion proof.
            const bool invalidated = result.current && demonstrablyImpossible( plans[*result.current] );
            const auto gate = compare( plan.score, plans[*reference].score );
            if ( index != result.current && !gate.accepted
                 && ( !invalidated || gate.reason == "priority_defense_degraded" || gate.reason == "comparison_unverified" ) ) { result.rejected.push_back( { index, gate.reason } ); continue; }
            bool accepted = true;
            for ( const size_t action : conversions ) {
                const auto & step = plan.candidate.actions[action];
                Proof proof; proof.plan = index; proof.action = action; proof.town = step.town; proof.day = step.day; proof.building = step.building;
                auto counterfactual = plan.candidate; counterfactual.actions.erase( counterfactual.actions.begin() + action );
                // Rebuild the dated graph with all other commitments/purchases preserved. If the
                // remaining schedule is illegal, no marginal benefit is inferred from that failure.
                if ( !work.merges ) proof.decision.reason = "shared_merge_limit";
                else {
                    --work.merges; const auto rebuilt = S::combine( *context, { &counterfactual }, plan.candidate.id );
                    if ( !rebuilt.valid ) proof.decision.reason = "counterfactual_rebuild_failed";
                    else {
                        const auto hit = cache.find( rebuilt.key );
                        if ( hit != cache.end() ) { proof.without = hit->second.score; proof.repairs = hit->second.repairs; proof.cached = true; ++result.cacheHits; }
                        else {
                            auto repaired = rebuilt.candidate;
                            // Remove no unrelated investment. Only cap optional purchases to the exact
                            // stock/affordability reported by C2 in ALL failing variants, then recheck the full plan.
                            // Every retry spends the same shared projection/day/force allowance.
                            while ( true ) {
                                if ( budget.projections < evaluator.requiredProjections() ) { proof.decision.reason = "shared_projection_preflight_limit"; break; }
                                const auto evaluated = evaluator.evaluate( repaired ); ++result.evaluations;
                                proof.without = compact( evaluated );
                                if ( proof.without.verified || evaluated.status == R::Status::Limit ) break;
                                P::Input materialized; if ( !G::materialize( *context, repaired, materialized ) ) break;
                                std::map<size_t, uint32_t> caps;
                                for ( const auto & variant : evaluated.variants ) {
                                    const auto & f = variant.evaluation.financial;
                                    std::optional<uint32_t> maximum;
                                    if ( f.reason == "recruit_stock_unavailable" && f.failedRecruitStock ) maximum = static_cast<uint32_t>( *f.failedRecruitStock );
                                    if ( f.reason == "payment_unavailable" && f.failedRecruitAffordable ) maximum = f.failedRecruitAffordable;
                                    if ( !maximum ) continue;
                                    for ( size_t i = 0; i < repaired.actions.size(); ++i ) {
                                        const auto & a = repaired.actions[i];
                                        const auto & resolved = materialized.actions[context->base.actions.size() + i];
                                        if ( resolved.id != f.failedAction || a.kind != P::Kind::Recruit || a.required ) continue;
                                        const auto cap = *maximum;
                                        if ( cap < a.count ) { const auto found = caps.find( i ); caps[i] = found == caps.end() ? cap : std::min( cap, found->second ); }
                                    }
                                }
                                if ( caps.empty() || !work.merges ) break;
                                --work.merges;
                                for ( auto it = caps.rbegin(); it != caps.rend(); ++it ) {
                                    auto & a = repaired.actions[it->first];
                                    proof.repairs.push_back( { a.day, a.town, a.pool, a.count, it->second } );
                                    if ( it->second ) a.count = it->second;
                                    else repaired.actions.erase( repaired.actions.begin() + it->first );
                                }
                            }
                            cache.emplace( rebuilt.key, Cached{ proof.without, proof.repairs } );
                        }
                        if ( proof.decision.reason.empty() ) proof.decision = compare( plan.score, proof.without );
                    }
                }
                accepted = accepted && proof.decision.accepted; result.proofs.push_back( std::move( proof ) );
                if ( !accepted ) break;
            }
            if ( accepted ) { result.selected = index; result.certified = true; break; }
            result.rejected.push_back( { index, result.proofs.back().decision.reason } );
        }
        if ( !result.selected && result.fallback ) { result.selected = result.fallback; result.certified = true; }
        result.gaps.erase( "role_stability_and_execution_pending" );
        result.gaps.insert( "live_execution_and_persistent_policy_pending" );
        if ( !result.selected ) result.gaps.insert( "no_stability_certified_plan" );
        return result;
    }
    Result observe( R::Session & evaluator, const G::Result & proposals, M::Budget & budget, S::Work & work, int player );
}
