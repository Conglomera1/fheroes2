// C3D5J. TEST-ONLY exact Peasant transfer policy; no normal AI connection.
#pragma once
#include "exact_recruit_fixture_adapter.h"

namespace TransferFixture
{
    namespace F = RecruitFixture;
    namespace P = F::P;
    namespace V = F::V;
    using Army = F::Army;
    struct Evidence : F::Evidence
    {
        std::tuple<uint64_t, Conscription::Quotas, Conscription::Quotas, uint32_t, uint32_t> progress{};
        auto key() const { return std::tuple_cat( F::Evidence::key(), std::tie( progress ) ); }
        bool operator==( const Evidence & b ) const { return key() == b.key(); }
    };
    inline Evidence capture( const Castle & town )
    {
        Evidence e; static_cast<F::Evidence &>( e ) = F::capture( town );
        const auto s = town.conscriptionDaySnapshot().state;
        e.progress = { s.total, s.quotas, s.used, s.next, s.lossRemainder }; return e;
    }
    inline uint64_t population( const Army & army, const bool peasantsOnly = false )
    {
        uint64_t result = 0;
        for ( const auto & slot : army ) if ( !peasantsOnly || slot.first == Monster::PEASANT ) result += slot.second;
        return result;
    }
    inline Army slots( const P::Army & input )
    { Army a{}; for ( size_t i = 0; i < a.size(); ++i ) a[i] = { input[i].monster, input[i].count }; return a; }
    inline bool supported( const P::Action & a ) { return a.kind == P::Kind::ToGarrison || a.kind == P::Kind::ToVisitor; }
    struct Checked { bool valid = false; P::Action action; Evidence before, expected; std::string reason; };
    inline Checked check( const Castle &, const P::Town & projected, const P::Action & a, const Evidence & before )
    {
        Checked c; c.action = a; c.before = c.expected = before;
        if ( !supported( a ) || !a.count || a.building || a.pool || a.town != before.town || a.day != before.day
             || before.race != Race::KNGT || projected.race != before.race || projected.tile != before.town || projected.mask != before.buildings
             || before.visitor < 0 || projected.hero != before.visitor || slots( projected.garrison ) != before.garrison
             || slots( projected.visitor ) != before.visitorArmy ) { c.reason = "transfer_shape_or_identity"; return c; }
        // Registry import supplies the fixed-visitor program binding; check alone
        // is never execution authority. Also require current physical fixedness.
        if ( !std::get<4>( before.visitorMission ) || std::get<5>( before.visitorMission ) != 0 ) { c.reason = "visitor_not_fixed"; return c; }
        const bool deposit = a.kind == P::Kind::ToGarrison;
        auto & source = deposit ? c.expected.visitorArmy : c.expected.garrison;
        auto & destination = deposit ? c.expected.garrison : c.expected.visitorArmy;
        if ( population( source, true ) < a.count ) { c.reason = "source_quantity"; return c; }
        if ( deposit && population( source ) <= a.count ) { c.reason = "last_hero_unit"; return c; }
        const int index = F::slot( destination, Monster::PEASANT, a.count );
        if ( index < 0 ) { c.reason = index == -2 ? "destination_overflow" : "destination_full"; return c; }
        uint32_t remaining = a.count;
        for ( auto & stack : source ) if ( stack.first == Monster::PEASANT && remaining ) {
            const auto take = std::min( remaining, stack.second ); stack.second -= take; remaining -= take;
            if ( !stack.second ) stack.first = 0;
        }
        if ( remaining ) { c.reason = "source_quantity"; return c; }
        auto & target = destination[static_cast<size_t>( index )];
        target = { Monster::PEASANT, target.second + a.count };
        c.valid = true; c.reason = "checked_exact_transfer"; return c;
    }
    inline V::Outcome classify( bool returned, const Evidence & before, const Checked & checked, const std::optional<Evidence> & after )
    {
        if ( after && returned && checked.valid && *after == checked.expected ) return V::Outcome::Executed;
        if ( after && !returned && *after == before ) return V::Outcome::Failed;
        return V::Outcome::Uncertain;
    }
    // Native exact primitive, deliberately isolated from broad legacy AI helpers.
    // Build/check both detached Troops copies before assigning either live army.
    // No cross-operation transaction promise: a post-write failure is Uncertain.
    inline bool apply( Castle & town, const P::Action & a, const Checked & checked )
    {
        if ( !checked.valid || !F::C::same( a, checked.action ) || !( capture( town ) == checked.before ) || !supported( a ) ) return false;
        auto * visitor = town.GetHero();
        if ( !visitor || visitor->GetID() != checked.before.visitor ) return false;
        ::Army & source = a.kind == P::Kind::ToGarrison ? visitor->GetArmy() : town.GetArmy();
        ::Army & destination = a.kind == P::Kind::ToGarrison ? town.GetArmy() : visitor->GetArmy();
        Troops sourceCopy( source ), destinationCopy( destination );
        uint32_t remaining = a.count;
        for ( size_t i = 0; i < sourceCopy.Size() && remaining; ++i ) {
            auto * troop = sourceCopy.GetTroop( i );
            if ( !troop || !troop->isValid() || troop->GetID() != Monster::PEASANT ) continue;
            const uint32_t take = std::min( remaining, troop->GetCount() ); remaining -= take;
            if ( take == troop->GetCount() ) troop->Reset(); else troop->SetCount( troop->GetCount() - take );
        }
        if ( remaining || !destinationCopy.JoinTroop( Monster::PEASANT, a.count, false ) ) return false;
        const auto & expectedSource = a.kind == P::Kind::ToGarrison ? checked.expected.visitorArmy : checked.expected.garrison;
        const auto & expectedDestination = a.kind == P::Kind::ToGarrison ? checked.expected.garrison : checked.expected.visitorArmy;
        if ( F::army( sourceCopy ) != expectedSource || F::army( destinationCopy ) != expectedDestination ) return false;
        source.Assign( sourceCopy ); destination.Assign( destinationCopy ); return true;
    }
    struct Policy
    {
        using Evidence = TransferFixture::Evidence;
        using Checked = TransferFixture::Checked;
        using Call = std::function<bool( Castle &, const P::Action &, const Checked & )>;
        static bool supports( const P::Action & a ) { return supported( a ); }
        static Evidence capture( const Castle & t ) { return TransferFixture::capture( t ); }
        static Checked check( const Castle & t, const P::Town & p, const P::Action & a, const Evidence & e ) { return TransferFixture::check( t, p, a, e ); }
        static V::Outcome classify( bool r, const Evidence & b, const Checked & c, const std::optional<Evidence> & a ) { return TransferFixture::classify( r, b, c, a ); }
        static bool dispatch( Castle & t, const P::Action & a, const Checked & c, const Call & callback ) { return callback ? callback( t, a, c ) : apply( t, a, c ); }
    };
    using Owner = F::ActionOwner<Policy>;
}
