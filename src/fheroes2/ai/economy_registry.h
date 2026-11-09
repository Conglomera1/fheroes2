// C3D5B: in-memory lifecycle for explicit observation programs; no engine callbacks yet.
#pragma once
#include "economy_commitments.h"

namespace AI::EconomyRegistry
{
    namespace C = EconomyCommitments;
    namespace G = EconomyCandidates;
    namespace P = EconomyProject;
    namespace X = EconomyMissions;
    enum class Status { Ready, Held, Invalid, Limit };
    enum class Outcome { Executed, Failed, Uncertain };
    struct Update { Status status = Status::Invalid; uint64_t project = 0; std::string reason; bool actionable = false; };
    // This ticket IDENTIFIES an expected operation; it does not authorize executing it.
    struct Ticket { uint64_t registry = 0; C::Stamp stamp; P::Action action; int32_t visitor = -1; };
    class Registry
    {
    public:
        // The future owner must issue a unique nonzero token per game/load/registry.
        Registry( const uint32_t player, const uint64_t token ) : player_( player ), token_( token ) {}
        Registry( const Registry & ) = delete;
        Registry & operator=( const Registry & ) = delete;
        Registry( Registry && ) = delete;
        Registry & operator=( Registry && ) = delete;
        bool ready() const { return ready_; }
        uint64_t revision() const { return revision_; }
        size_t pending() const
        { size_t count = 0; for ( const auto & p : book_.programs ) count += p.pending.size(); return count; }
        int32_t boundVisitor( const uint64_t project ) const
        { for ( const auto & p : book_.programs ) if ( p.id == project ) return p.visitor; return -1; }

        Update refresh( const G::Context & fresh, const X::Snapshot & missions )
        {
            // ANY attempted refresh makes old tickets unusable, including a failed recapture.
            ready_ = false;
            if ( !advanceRevision() ) return { Status::Limit, 0, "registry_revision_exhausted" };
            if ( !player_ || !token_ || !fresh.base.actions.empty() || !fresh.active.empty() )
                return { Status::Invalid, 0, "registry_requires_owned_fresh_snapshot" };
            if ( uncertain_ ) return { Status::Held, 0, "uncertain_execution_requires_reconciliation" };
            const auto day = fresh.base.start.economy.day;
            if ( day < book_.stamp.day ) return { Status::Held, 0, "registry_day_rewind_requires_new_instance" };
            auto book = book_; book.stamp = { player_, day, revision_ };
            const auto imported = C::import( fresh, missions, book.stamp, book );
            if ( !imported.context ) return { Status::Held, 0, imported.reason };
            fresh_ = fresh; missions_ = missions; book_ = std::move( book ); ready_ = true;
            return { Status::Ready, 0, "registry_refreshed_not_funded" };
        }

        // Produces stable IDs from explicit actions, never from a legacy priority or budget hint.
        // Draft project/action IDs must be zero; dependencies reference existing registry projects.
        Update stage( C::Program draft )
        {
            if ( !ready_ || !fresh_ || !missions_ ) return { Status::Held, 0, "registry_refresh_required" };
            if ( draft.id || draft.executedPrefix || draft.pending.empty() ) return { Status::Invalid, 0, "invalid_registry_draft" };
            if ( book_.programs.size() >= 64 || draft.pending.size() > 1024 - pending() ) return { Status::Limit, 0, "registry_schedule_limit" };
            if ( revision_ == UINT64_MAX || nextProject_ >= idLimit || draft.pending.size() > idLimit - nextAction_ )
                return { Status::Limit, 0, "registry_identity_exhausted" };
            const auto project = nextProject_; auto nextAction = nextAction_;
            draft.id = project;
            for ( auto & action : draft.pending ) {
                if ( action.id || action.project ) return { Status::Invalid, 0, "registry_draft_already_identified" };
                action.id = nextAction++; action.project = project;
            }
            auto book = book_; book.stamp.revision = revision_ + 1; book.programs.push_back( std::move( draft ) );
            const auto imported = C::import( *fresh_, *missions_, book.stamp, book );
            if ( !imported.context ) return { imported.status == C::Status::Limit ? Status::Limit : Status::Invalid, 0, imported.reason };
            book_ = std::move( book ); ++revision_; ++nextProject_; nextAction_ = nextAction;
            return { Status::Ready, project, "registry_program_staged_not_funded" };
        }

        C::Result view() const
        {
            if ( ready_ && fresh_ && missions_ ) return C::import( *fresh_, *missions_, book_.stamp, book_ );
            C::Result held; held.reason = "registry_refresh_required"; return held;
        }

        std::optional<Ticket> ticket( const uint64_t actionId ) const
        {
            if ( !ready_ ) return {};
            for ( const auto & p : book_.programs ) if ( p.dependsOn.empty() && !p.pending.empty()
                && p.pending.front().id == actionId && p.pending.front().day == book_.stamp.day )
                return Ticket{ token_, book_.stamp, p.pending.front(), p.visitor };
            return {};
        }

        Update record( const Ticket & receipt, const Outcome outcome )
        {
            const auto expected = ticket( receipt.action.id );
            if ( !expected || receipt.registry != token_ || receipt.visitor != expected->visitor
                 || !C::sameStamp( receipt.stamp, expected->stamp ) || !C::same( receipt.action, expected->action ) )
                return { Status::Invalid, 0, "stale_or_conflicting_registry_receipt" };
            if ( outcome != Outcome::Executed && outcome != Outcome::Failed && outcome != Outcome::Uncertain )
                return { Status::Invalid, 0, "invalid_registry_outcome" };
            ready_ = false;
            if ( !advanceRevision() ) return { Status::Limit, 0, "registry_revision_exhausted" };
            const auto project = receipt.action.project;
            if ( outcome != Outcome::Executed ) {
                // An uncertain operation might already have spent resources. A new raw
                // snapshot alone cannot prove which step happened: forbid automatic retry.
                uncertain_ = outcome == Outcome::Uncertain;
                return { Status::Held, project, uncertain_ ? "uncertain_execution_requires_reconciliation" : "failed_action_retained_refresh_required" };
            }
            const auto it = std::find_if( book_.programs.begin(), book_.programs.end(), [&]( const auto & p ) { return p.id == project; } );
            it->pending.erase( it->pending.begin() ); it->executedPrefix = true;
            if ( it->pending.empty() ) {
                book_.programs.erase( it );
                for ( auto & p : book_.programs ) p.dependsOn.erase( project );
            }
            // Never replay a payment, infer a conquest or edit funds/armies here.
            // The confirmation must come from actual success, not a projected schedule.
            return { Status::Held, project, "execution_recorded_refresh_required" };
        }

    private:
        bool advanceRevision()
        { if ( revision_ == UINT64_MAX ) return false; ++revision_; return true; }
        static constexpr uint64_t idLimit = uint64_t{1} << 63;
        const uint32_t player_;
        const uint64_t token_;
        uint64_t revision_ = 0, nextProject_ = 1, nextAction_ = 1;
        bool ready_ = false;
        bool uncertain_ = false;
        C::Book book_;
        std::optional<G::Context> fresh_;
        std::optional<X::Snapshot> missions_;
    };
}
