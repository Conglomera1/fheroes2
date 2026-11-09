// C3D3: observation-only coordination of one immutable planning frame.
#pragma once
#include "economy_inputs.h"
#include "economy_recourse.h"
#include "economy_stability.h"
#include "economy_registry_observation.h"

namespace AI::EconomyPipeline
{
    namespace I = EconomyInputs;
    namespace G = EconomyCandidates;
    namespace M = EconomyMilitary;
    namespace D = EconomyDefense;
    namespace Q = EconomyRecourse;
    namespace T = EconomyStability;
    struct Work { I::Work paths; G::Budget generation; M::Budget evaluation; EconomySearch::Work search; };
    enum class Status { Observed, Partial, Held, Invalid };
    struct Result
    {
        Status status = Status::Invalid;
        I::Frame frame;
        G::Result proposals;
        Q::Result preparation;
        T::Result selection;
        EconomyRegistryObservation::Result registry;
        size_t generationPasses = 0;
        bool widened = false, searched = false, actionable = false;
        std::set<std::string> gaps;
        std::string reason;
    };
    template<class Select>
    inline Result evaluate( I::Frame frame, Work & work, const M::Scorer & scorer, const D::Scorer & defender, Select select )
    {
        Result out;
        for ( size_t pass = 0; pass < 2; ++pass ) {
            if ( !frame.valid ) { out.reason = "invalid_input_frame"; out.gaps = frame.gaps; out.frame = std::move( frame ); return out; }
            if ( !frame.gaps.empty() || !frame.defenses.coverageComplete || !frame.risks.coverageComplete
                 || !frame.risks.gaps.empty() || std::any_of( frame.assignments.begin(), frame.assignments.end(), []( const auto & a ) { return !a.verified; } ) ) {
                out.status = Status::Held; out.reason = "planner_evidence_incomplete"; out.gaps = frame.gaps;
                out.gaps.insert( frame.risks.gaps.begin(), frame.risks.gaps.end() ); out.frame = std::move( frame ); return out;
            }
            out.proposals = G::generate( frame.context, work.generation ); ++out.generationPasses;
            if ( !out.proposals.valid ) { out.status = Status::Held; out.reason = "candidate_generation_invalid"; out.frame = std::move( frame ); return out; }
            const auto & input = frame.context.base;
            const auto afterHorizon = [&]( const auto & actions ) {
                return std::any_of( actions.begin(), actions.end(), [&]( const auto & a ) { return a.day > input.throughDay; } );
            };
            const bool datedContinuation = afterHorizon( input.actions ) || afterHorizon( frame.context.active );
            if ( uint64_t{ input.start.economy.day } + 14 == input.throughDay && ( out.proposals.longerHorizonNeeded || frame.outsideHorizon || datedContinuation ) ) {
                // Same captured state, INI, enemy knowledge and cached path evidence.
                // Discard ALL old proposals; no old score is compared to a wider one.
                frame = I::widen( frame, work.paths ); out.widened = true; continue;
            }
            if ( frame.outsideHorizon || datedContinuation ) {
                out.proposals.longerHorizonNeeded = true; out.proposals.gaps.insert( "known_threat_or_commitment_beyond_detailed_horizon" );
            }
            out.frame = std::move( frame );
            // No live commitment is declared revisable here; no mission or authority is invented.
            out.preparation = Q::prepare( out.frame.context, {}, out.frame.assignments, scorer, work.evaluation,
                                          out.frame.defenses, defender, out.frame.risks );
            if ( out.preparation.status != Q::Status::Ready ) {
                out.status = Status::Held; out.reason = out.preparation.reason; return out;
            }
            out.selection = select( *out.preparation.session, out.proposals, work.evaluation, work.search ); out.searched = true;
            if ( !out.selection.selected || !out.selection.certified ) {
                out.status = Status::Held; out.reason = "no_verified_observation_choice"; return out;
            }
            out.status = out.selection.search.status == EconomySearch::Status::Ready ? Status::Observed : Status::Partial;
            out.gaps = out.selection.gaps; out.reason = "hypothetical_choice_never_executed"; return out;
        }
        out.frame = std::move( frame ); out.status = Status::Held; out.reason = "common_horizon_not_ready"; return out;
    }
    inline Result evaluate( I::Frame frame, Work & work, const M::Scorer & scorer, const D::Scorer & defender )
    {
        return evaluate( std::move( frame ), work, scorer, defender,
            []( auto & session, const auto & proposals, auto & budget, auto & search ) { return T::select( session, proposals, budget, search ); } );
    }
    Result observe( I::Frame, const Kingdom &, Work & );
}
