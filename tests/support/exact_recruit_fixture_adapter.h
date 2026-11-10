// C3D5H. TEST-ONLY owner for a generated, single-town, no-threat fixture.
// Not included by production. No observer promotion or general execution authority.
#pragma once
#include "army_troop.h"
#include "economy_registry.h"
#include "heroes.h"
#include "kingdom.h"
#include "world.h"
#include <functional>
#include <tuple>

namespace RecruitFixture
{
    namespace V = AI::EconomyRegistry;
    namespace C = AI::EconomyCommitments;
    namespace P = AI::EconomyProject;
    namespace G = AI::EconomyCandidates;
    namespace X = AI::EconomyMissions;
    namespace M = AI::EconomyMilitary;
    namespace D = AI::EconomyDefense;
    namespace R = AI::EconomyRisk;
    namespace Q = AI::EconomyRecourse;
    using Army = std::array<std::pair<int, uint32_t>, 5>;
    using Resources = P::Resources;
    constexpr std::array<uint32_t, 6> bases{ DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3,
                                           DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };
    inline Resources resources( const Funds & f ) { return { f.gold, f.wood, f.ore, f.mercury, f.sulfur, f.crystal, f.gems }; }
    inline Army army( const Troops & source )
    {
        Army out{};
        for ( size_t i = 0; i < out.size(); ++i ) {
            const auto * t = source.GetTroop( i );
            if ( t && t->isValid() ) out[i] = { t->GetID(), t->GetCount() };
        }
        return out;
    }
    inline auto configKey()
    {
        const auto & c = Conscription::settings();
        return std::make_tuple( c.goldPerPeasant, c.quotas, c.weeklyLimit, c.dailyLimit, c.oddDayLimit, c.evenDayLimit,
            c.enabled, c.successRate, c.requiredBuildings, c.trainingGold, c.trainingWood, c.trainingOre,
            c.farmGold, c.farmWood, c.farmOre, Kingdom::GetTaxpayerGoldPerPeasantPerDay(), Kingdom::TaxpayerRequiresFarms() );
    }
    struct Evidence
    {
        Resources money{};
        std::array<uint32_t, 6> stock{};
        Army garrison{}, visitorArmy{};
        int32_t town = -1, visitor = -1;
        int player = 0, race = 0;
        uint32_t day = 0, buildings = 0, disabled = 0;
        bool canBuild = false;
        // Physical mission facts must remain stable across prepare/call/receipt too.
        std::tuple<int, int32_t, int32_t, uint32_t, bool, uint32_t, bool, int> visitorMission{};
        decltype( configKey() ) config{};
        auto key() const { return std::tie( money, stock, garrison, visitorArmy, town, visitor, player, race, day, buildings, disabled, canBuild, visitorMission, config ); }
        bool operator==( const Evidence & b ) const { return key() == b.key(); }
    };
    inline Evidence capture( const Castle & town )
    {
        Evidence e; e.money = resources( town.GetKingdom().GetFunds() ); e.town = town.GetIndex();
        e.player = static_cast<int>( town.GetColor() ); e.race = town.GetRace(); e.day = world.CountDay();
        e.canBuild = town.Modes( Castle::ALLOW_TO_BUILD_TODAY ); e.config = configKey(); e.garrison = army( town.GetArmy() );
        if ( const auto * h = town.GetHero() ) {
            e.visitor = h->GetID(); e.visitorArmy = army( h->GetArmy() );
            e.visitorMission = { static_cast<int>( h->GetColor() ), h->GetIndex(), h->GetPath().GetDestinationIndex(), h->GetMovePoints(),
                                 h->Modes( Heroes::PATROL ), h->GetPatrolDistance(), h->Modes( Heroes::SLEEPER ), static_cast<int>( h->getAIRole() ) };
        }
        for ( size_t i = 0; i < bases.size(); ++i ) e.stock[i] = town.getMonstersInDwelling( bases[i] );
        for ( uint32_t bit = 1; bit; bit <<= 1 ) {
            if ( town.isBuild( bit ) ) e.buildings |= bit;
            if ( town.isBuildingDisabled( bit ) ) e.disabled |= bit;
        }
        return e;
    }
    struct Checked { bool valid = false; int monster = 0; bool toVisitor = false; Resources cost{}; Evidence expected; std::string reason; };
    // Pure range helpers also allow independent synthetic tests for each resource.
    inline bool checkedCost( const Resources & unit, const uint32_t count, const Resources & funds, Resources & total )
    {
        if ( !count || !AI::EconomyBudget::validAmounts( unit ) || !AI::EconomyBudget::validAmounts( funds ) ) return false;
        for ( size_t i = 0; i < 7; ++i ) {
            if ( unit[i] > AI::EconomyBudget::resourceMaximum / count ) return false;
            total[i] = unit[i] * count;
            if ( total[i] > funds[i] ) return false;
        }
        return true;
    }
    // -1 means no slot; -2 means the engine's preferred slot would overflow.
    inline int slot( const Army & a, const int monster, const uint32_t count )
    {
        for ( size_t i = 0; i < a.size(); ++i ) if ( a[i].second && a[i].first == monster )
            return uint64_t{ a[i].second } + count > UINT32_MAX ? -2 : static_cast<int>( i );
        for ( size_t i = 0; i < a.size(); ++i ) if ( !a[i].second ) return static_cast<int>( i );
        return -1;
    }
    inline Checked check( const Castle & town, const P::Town & projected, const P::Action & action, const Evidence & before )
    {
        Checked c; c.expected = before;
        if ( action.kind != P::Kind::Recruit || action.pool >= 6 || !action.count || action.building
             || action.town != before.town || action.day != before.day || projected.tile != before.town
             || projected.race != before.race || projected.mask != before.buildings ) { c.reason = "action_shape"; return c; }
        const auto pool = action.pool;
        if ( !( before.buildings & bases[pool] ) || action.count > before.stock[pool] ) { c.reason = "stock"; return c; }
        const Monster monster( town.GetRace(), town.GetActualDwelling( bases[pool] ) );
        const auto & creature = projected.dwellings[pool].creature( projected.mask );
        c.monster = monster.GetID();
        if ( !monster.isValid() || monster.GetRace() != town.GetRace() || c.monster != creature.id
             || resources( monster.GetCost() ) != creature.price ) { c.reason = "creature_or_price"; return c; }
        if ( !checkedCost( resources( monster.GetCost() ), action.count, before.money, c.cost ) ) { c.reason = "cost_or_funds"; return c; }
        int index = slot( before.garrison, c.monster, action.count );
        if ( index == -2 ) { c.reason = "stack_overflow"; return c; }
        if ( index == -1 ) {
            if ( before.visitor < 0 ) { c.reason = "destination"; return c; }
            c.toVisitor = true; index = slot( before.visitorArmy, c.monster, action.count );
        }
        if ( index < 0 ) { c.reason = index == -2 ? "stack_overflow" : "destination"; return c; }
        auto & destination = c.toVisitor ? c.expected.visitorArmy : c.expected.garrison;
        destination[static_cast<size_t>( index )] = { c.monster, destination[static_cast<size_t>( index )].second + action.count };
        c.expected.stock[pool] -= action.count;
        for ( size_t i = 0; i < 7; ++i ) c.expected.money[i] -= c.cost[i];
        c.valid = true; c.reason = "checked"; return c;
    }
    inline V::Outcome classify( const bool returned, const Evidence & before, const Checked & checked, const std::optional<Evidence> & after )
    {
        if ( after && checked.valid && returned && *after == checked.expected ) return V::Outcome::Executed;
        if ( after && !returned && *after == before ) return V::Outcome::Failed;
        return V::Outcome::Uncertain;
    }
    enum class Status { Held, Prepared, Executed, Failed, Blocked, Empty };
    struct Result { Status status = Status::Held; std::string reason; std::optional<V::Ticket> ticket; };

    struct RecruitPolicy
    {
        using Evidence = RecruitFixture::Evidence;
        using Checked = RecruitFixture::Checked;
        using Call = std::function<bool( Castle &, const Troop & )>;
        static bool supports( const P::Action & a ) { return a.kind == P::Kind::Recruit; }
        static Evidence capture( const Castle & t ) { return RecruitFixture::capture( t ); }
        static Checked check( const Castle & t, const P::Town & p, const P::Action & a, const Evidence & e )
        { return RecruitFixture::check( t, p, a, e ); }
        static V::Outcome classify( bool returned, const Evidence & before, const Checked & checked, const std::optional<Evidence> & after )
        { return RecruitFixture::classify( returned, before, checked, after ); }
        static bool dispatch( Castle & t, const P::Action & a, const Checked & c, const Call & testCall )
        { const Troop request( c.monster, a.count ); return testCall ? testCall( t, request ) : t.RecruitMonster( request, false ); }
    };

    // Only instantiated in the explicitly generated no-threat native fixture.
    // C3D5I shares only lifecycle; policies explicitly bound their supported effects.
    // One accepted program, one town, current-day actions. No add/replace API.
    template<class Policy> class ActionOwner
    {
        using Evidence = typename Policy::Evidence;
        using Checked = typename Policy::Checked;
    public:
        ActionOwner( Kingdom & k, Castle & town, const uint64_t token )
            : kingdom_( k ), town_( town ), registry_( static_cast<uint32_t>( k.GetColor() ), token ), token_( token ) {}
        bool acceptProgram( C::Program program )
        {
            if ( accepted_ || blocked_ || !token_ || program.town != town_.GetIndex() || program.pending.empty() || program.pending.size() > 16
                 || program.id || program.executedPrefix || program.allowAbandon || program.independent || !program.dependsOn.empty()
                 || program.source != C::Source::ExplicitProgram || program.destination != C::Destination::Town || program.hero != -1 ) return false;
            for ( const auto & a : program.pending ) if ( !Policy::supports( a ) || a.day != world.CountDay() ) return false;
            auto frame = fresh();
            if ( !frame || registry_.refresh( frame->first, frame->second ).status != V::Status::Ready ) return false;
            if ( registry_.stage( std::move( program ) ).status != V::Status::Ready ) return false;
            accepted_ = true; acceptedDay_ = world.CountDay(); acceptedVisitor_ = Policy::capture( town_ ).visitor; return true;
        }
        Result prepare( M::Budget & budget, const bool fixtureThreatCoverageComplete = true )
        {
            prepared_.reset();
            if ( blocked_ || inflight_ ) return { Status::Blocked, "owner_blocked_or_inflight", {} };
            if ( !accepted_ ) return { Status::Held, "explicit_acceptance_required", {} };
            auto frame = fresh();
            if ( !frame || world.CountDay() != acceptedDay_ || Policy::capture( town_ ).visitor != acceptedVisitor_ )
                return { Status::Held, "fixture_identity_changed", {} };
            const auto refreshed = registry_.refresh( frame->first, frame->second );
            if ( refreshed.status != V::Status::Ready ) return { Status::Held, refreshed.reason, {} };
            const auto view = registry_.view();
            if ( !view.context ) return { Status::Held, view.reason, {} };
            if ( view.context->base.actions.empty() ) return { Status::Empty, "complete", {} };
            // Single accepted program preserves its exact global order by construction.
            const auto ticket = registry_.ticket( view.context->base.actions.front().id );
            if ( !ticket || registry_.revision() == UINT64_MAX ) return { Status::Held, "ticket_unavailable", {} };
            const auto before = Policy::capture( town_ );
            const auto checked = Policy::check( town_, frame->first.base.start.towns.front(), ticket->action, before );
            if ( !checked.valid ) return finish( *ticket, V::Outcome::Failed, checked.reason );
            std::vector<M::Assignment> uses{ { 1, town_.GetIndex(), M::Slot::TownArmy, before.day, before.day + 14, true, true } };
            // Complete empty threat coverage is an explicit GENERATED-FIXTURE premise.
            // Actual Project/Risk/Recourse use the caller's SAME counters and real scorers.
            const auto verification = Q::prepare( *view.context, {}, uses,
                [&]( const auto & t, const auto s ) { return M::localArmyValue( kingdom_, t, s ); }, budget,
                D::Registry{ {}, fixtureThreatCoverageComplete },
                [&]( const auto & t, const auto & threat ) { return D::localDefenseValue( kingdom_, t, threat ); },
                R::Registry{ {}, fixtureThreatCoverageComplete, {} } );
            if ( verification.status != Q::Status::Ready || !verification.session || !verification.released.empty() )
                return { Status::Held, verification.reason, {} };
            prepared_ = Prepared{ *ticket, before, checked };
            return { Status::Prepared, "fixture_schedule_verified", ticket };
        }
        // Optional callback is a TEST fault seam, never supplied by production code.
        using Call = typename Policy::Call;
        Result run( const V::Ticket & ticket, const Call & testCall = {} )
        {
            if ( blocked_ || inflight_ ) return { Status::Blocked, "owner_blocked_or_inflight", {} };
            if ( !accepted_ || !prepared_ || !same( ticket, prepared_->ticket ) ) return { Status::Held, "no_matching_preparation", {} };
            const auto current = registry_.ticket( ticket.action.id );
            if ( !current || !same( ticket, *current ) ) { prepared_.reset(); return { Status::Held, "stale_ticket", {} }; }
            if ( !( Policy::capture( town_ ) == prepared_->before ) ) { prepared_.reset(); return { Status::Held, "physical_state_changed", {} }; }
            auto operation = *prepared_; prepared_.reset(); inflight_ = true;
            V::Outcome outcome = V::Outcome::Uncertain;
            try {
                ++calls_;
                const bool returned = Policy::dispatch( town_, ticket.action, operation.checked, testCall );
                outcome = Policy::classify( returned, operation.before, operation.checked, Policy::capture( town_ ) );
            }
            catch ( ... ) { outcome = V::Outcome::Uncertain; }
            inflight_ = false;
            return finish( ticket, outcome, "engine_result" );
        }
        size_t pending() const { return registry_.pending(); }
        size_t calls() const { return calls_; }
        bool blocked() const { return blocked_; }
        // Used only to inject post-effect stale-record faults in the native test.
        void invalidateForTest() { auto f = fresh(); if ( f ) registry_.refresh( f->first, f->second ); }

    private:
        using Frame = std::pair<G::Context, X::Snapshot>;
        struct Prepared { V::Ticket ticket; Evidence before; Checked checked; };
        static bool same( const V::Ticket & a, const V::Ticket & b )
        { return a.registry == b.registry && a.visitor == b.visitor && C::sameStamp( a.stamp, b.stamp ) && C::same( a.action, b.action ); }
        std::optional<Frame> fresh() const
        {
            if ( kingdom_.GetCastles().size() != 1 || kingdom_.GetCastles().front() != &town_ || town_.GetColor() != kingdom_.GetColor()
                 || world.CountDay() > UINT32_MAX - 14 ) return {};
            if ( kingdom_.GetHeroes().size() > 1 ) return {};
            for ( const auto * hero : kingdom_.GetHeroes() ) if ( !hero || hero != town_.GetHero() ) return {};
            G::Context c; c.base = P::capture( kingdom_, world.CountDay() + 14 );
            auto missions = X::capture( kingdom_, c.base.start, {}, {} );
            if ( c.base.start.towns.size() != 1 || !missions.valid || !missions.gaps.empty() ) return {};
            return Frame{ std::move( c ), std::move( missions ) };
        }
        Result finish( const V::Ticket & ticket, const V::Outcome outcome, const std::string & reason )
        {
            // Fail closed even if recording itself throws after the engine has acted.
            blocked_ = true;
            const size_t pendingBefore = registry_.pending();
            if ( !pendingBefore ) return { Status::Blocked, "missing_pending_reconciliation_required", {} };
            V::Update recorded;
            try { recorded = registry_.record( ticket, outcome ); }
            catch ( ... ) { return { Status::Blocked, "record_exception_reconciliation_required", {} }; }
            const std::string expected = outcome == V::Outcome::Executed ? "execution_recorded_refresh_required"
                : outcome == V::Outcome::Failed ? "failed_action_retained_refresh_required" : "uncertain_execution_requires_reconciliation";
            if ( recorded.status != V::Status::Held || recorded.project != ticket.action.project || recorded.reason != expected
                 || registry_.ready() || registry_.pending() != pendingBefore - ( outcome == V::Outcome::Executed ? 1 : 0 ) ) {
                blocked_ = true; return { Status::Blocked, "record_rejected_reconciliation_required", {} };
            }
            if ( outcome == V::Outcome::Uncertain ) { blocked_ = true; return { Status::Blocked, expected, {} }; }
            blocked_ = false;
            return { outcome == V::Outcome::Executed ? Status::Executed : Status::Failed, reason, {} };
        }
        Kingdom & kingdom_; Castle & town_; V::Registry registry_; const uint64_t token_;
        bool accepted_ = false, blocked_ = false, inflight_ = false;
        uint32_t acceptedDay_ = 0; int32_t acceptedVisitor_ = -1;
        size_t calls_ = 0; std::optional<Prepared> prepared_;
    };
    using Owner = ActionOwner<RecruitPolicy>;
}
