// C3C3: dated funding evidence, never virtual money or a live spending permit.
#pragma once
#include "economy_risk.h"

namespace AI::EconomyReserve
{
    namespace B = EconomyBudget;
    namespace P = EconomyProject;
    namespace G = EconomyCandidates;
    namespace R = EconomyRisk;
    struct Variant
    {
        uint64_t scenario = 0;
        bool funded = false;
        uint32_t failedDay = 0;
        uint64_t failedAction = 0;
        std::string reason;
        std::optional<P::Resources> observedMinimum;
    };
    struct Assessment
    {
        bool protectedTrajectory = false;
        bool additionalNeedKnown = false;
        P::Resources additionalNeed{}; // Zero is certified only when additionalNeedKnown.
        std::vector<Variant> variants;
        std::string reason = "actual_projection_incomplete";
    };
    inline Assessment assess( const R::Result & result )
    {
        Assessment out;
        bool covered = result.status == R::Status::Scored && !result.variants.empty();
        std::set<uint64_t> ids;
        for ( const auto & variant : result.variants ) {
            const auto & f = variant.evaluation.financial;
            const bool paid = f.covered() && f.budget.covered() && f.cancelledRequiredActions.empty();
            covered = covered && paid && ids.insert( variant.scenario ).second;
            out.variants.push_back( { variant.scenario, paid, f.failedDay, f.failedAction, f.reason, variant.minimumFunds } );
        }
        covered = covered && result.variants.front().scenario == 0;
        out.protectedTrajectory = covered;
        if ( covered ) { out.additionalNeedKnown = true; out.reason = "all_retained_payments_funded_with_real_money"; }
        else if ( result.status == R::Status::Exposed ) out.reason = "required_variant_exposed";
        else if ( result.status == R::Status::Limit ) out.reason = "shared_work_limit";
        else if ( result.status == R::Status::Invalid ) out.reason = "invalid_projection_context";
        return out;
    }
    struct Commitments
    {
        bool valid = false;
        std::vector<P::Action> retained; // Materialized IDs, common payments included exactly once.
        std::vector<P::Action> beyondHorizon;
        std::optional<uint32_t> nextMilestone;
        std::string reason;
    };
    inline Commitments commitments( const G::Context & context, const G::Candidate & candidate )
    {
        Commitments out; P::Input input;
        if ( context.base.actions.size() > 1024 || candidate.actions.size() > 1024 ) { out.reason = "commitment_input_limit"; return out; }
        if ( !G::materialize( context, candidate, input ) ) { out.reason = "invalid_commitment_identity"; return out; }
        // Base payments are retained obligations. New required steps join them;
        // optional steps still have to fit, but are not separately reserved again.
        for ( size_t i = 0; i < input.actions.size(); ++i ) {
            const auto & action = input.actions[i];
            if ( i < context.base.actions.size() || action.required ) out.retained.push_back( action );
            if ( action.day > input.throughDay ) {
                out.beyondHorizon.push_back( action );
                if ( !out.nextMilestone || action.day < *out.nextMilestone ) out.nextMilestone = action.day;
            }
        }
        out.valid = true;
        out.reason = out.beyondHorizon.empty() ? "identities_preserved_not_a_second_debit" : "future_milestone_requires_common_review";
        return out;
    }
    struct EnvelopeInput
    {
        B::Input ledger;
        Conscription::Settings config;
        uint32_t peasantCost = 0;
        std::vector<B::TrainingPeriod> training;
        // Caller must certify the dated fixed income, schedule and ownership in
        // THIS scenario. Do not infer fixed future taxes from a cash-dependent TG replay.
        bool fixedFlowsVerified = false, scheduleVerified = false;
    };
    struct Envelope
    {
        bool complete = false, conservative = false;
        P::Resources sufficientAdditional{}; // Upper envelope, NOT a minimum or funds available.
        uint32_t firstShortfallDay = 0;
        uint64_t firstShortfall = 0;
        std::string reason;
    };
    inline Envelope envelope( const EnvelopeInput & supplied )
    {
        Envelope out;
        if ( !supplied.fixedFlowsVerified || !supplied.scheduleVerified ) { out.reason = "fixed_flows_or_schedule_unverified"; return out; }
        // C1 validates even nested claims before the copy and limits its own work.
        const auto skeleton = B::evaluate( supplied.ledger );
        if ( skeleton.status != B::Status::Covered && skeleton.status != B::Status::Unfunded ) { out.reason = skeleton.reason; return out; }
        for ( const auto & morning : supplied.ledger.mornings )
            if ( morning.automaticBound != P::Resources{} || morning.conservative ) { out.reason = "automatic_charge_already_supplied"; return out; }
        auto ledger = supplied.ledger;
        const auto bound = B::conscriptionBound( supplied.config, supplied.peasantCost, ledger.today, ledger.throughDay, supplied.training );
        if ( !bound.complete ) { out.reason = bound.reason; return out; }
        for ( const auto & day : bound.days ) {
            if ( !day.second ) continue;
            out.conservative = true;
            auto it = std::find_if( ledger.mornings.begin(), ledger.mornings.end(), [&]( const auto & m ) { return m.day == day.first; } );
            if ( it == ledger.mornings.end() ) {
                std::set<uint64_t> ids;
                for ( const auto & m : ledger.mornings ) ids.insert( m.id );
                for ( const auto & s : ledger.steps ) ids.insert( s.id );
                uint64_t id = uint64_t{ 1 } << 63;
                while ( ids.count( id ) ) ++id;
                B::Morning m; m.id = id; m.day = day.first; ledger.mornings.push_back( m ); it = ledger.mornings.end() - 1;
            }
            it->automaticBound[0] = day.second; it->conservative = true;
        }
        const auto audit = B::evaluate( ledger );
        if ( audit.status != B::Status::Covered && audit.status != B::Status::Unfunded && audit.status != B::Status::ConservativeGap ) {
            out.reason = audit.reason; return out;
        }
        // Adding a grant above the game resource cap is not a realizable sufficient envelope.
        for ( size_t r = 0; r < 7; ++r ) if ( audit.extraNeeded[r] > B::resourceMaximum - ledger.funds[r] ) {
            out.reason = "envelope_outside_resource_capacity"; return out;
        }
        out.complete = true; out.sufficientAdditional = audit.extraNeeded;
        out.firstShortfallDay = audit.firstShortfallDay; out.firstShortfall = audit.firstShortfall;
        out.reason = out.conservative ? "maximum_fees_envelope_not_exact_minimum" : "fixed_flow_prefix_envelope";
        return out;
    }
    struct Refinement
    {
        Assessment actual;
        bool envelopeKnown = false;
        P::Resources sufficientAdditional{};
        std::map<uint64_t, Envelope> envelopes;
        std::string reason;
        bool actionable = false;
    };
    inline Refinement refine( const R::Result & exact, const std::map<uint64_t, EnvelopeInput> & supplied )
    {
        Refinement out; out.actual = assess( exact );
        // An excessive bound cannot veto a trajectory already financed exactly.
        if ( out.actual.protectedTrajectory ) { out.envelopeKnown = true; out.reason = "exact_replay_needs_no_additional_reserve"; return out; }
        if ( exact.status == R::Status::Invalid || exact.status == R::Status::Limit || exact.variants.empty() ) {
            out.reason = "actual_result_unavailable_no_reserve_claim"; return out;
        }
        bool known = exact.status == R::Status::Exposed;
        std::set<uint64_t> ids;
        for ( const auto & variant : exact.variants ) {
            const auto found = supplied.find( variant.scenario );
            if ( !ids.insert( variant.scenario ).second || found == supplied.end() ) { known = false; continue; }
            auto value = envelope( found->second ); known = known && value.complete;
            for ( size_t r = 0; r < 7; ++r ) out.sufficientAdditional[r] = std::max( out.sufficientAdditional[r], value.sufficientAdditional[r] );
            out.envelopes.emplace( variant.scenario, std::move( value ) );
        }
        out.envelopeKnown = known;
        out.reason = known ? "conditional_envelope_only_actual_plan_still_unprotected" : "reserve_conservative_or_incomplete_need_unresolved";
        return out;
    }
}
