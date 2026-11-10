// C3D5I. Test-only Knight Farm/TG policy on the shared isolated action lifecycle.
// No dwelling, captain, spell, map or general building execution is supported.
#pragma once
#include "exact_recruit_fixture_adapter.h"
#include "castle_building_info.h"

namespace BuildFixture
{
    namespace F = RecruitFixture;
    namespace P = F::P;
    namespace V = F::V;
    using Resources = F::Resources;
    struct Evidence : F::Evidence
    {
        Resources income{};
        bool snapshotValid = false, farm = false, training = false;
        uint32_t hutGrowth = 0;
        std::array<uint32_t, 3> poolGrowth{};
        int64_t tax = 0;
        // Training progress must survive a construction/conversion unchanged.
        std::tuple<uint64_t, Conscription::Quotas, Conscription::Quotas, uint32_t, uint32_t> progress{};
        auto key() const { return std::tuple_cat( F::Evidence::key(), std::tie( income, snapshotValid, farm, training, hutGrowth, poolGrowth, tax, progress ) ); }
        bool operator==( const Evidence & b ) const { return key() == b.key(); }
    };
    inline Evidence capture( const Castle & town )
    {
        Evidence e; static_cast<F::Evidence &>( e ) = F::capture( town );
        const auto snapshot = town.GetKingdom().economySnapshot();
        e.income = F::resources( town.GetKingdom().GetIncome( Kingdom::INCOME_ALL ) ); e.tax = Economy::taxIncome( snapshot );
        if ( snapshot.towns.size() != 1 || snapshot.towns[0].tile != town.GetIndex() ) return e;
        const auto & t = snapshot.towns[0]; const auto & s = t.troops.state;
        e.snapshotValid = true; e.farm = t.farm; e.training = t.training; e.hutGrowth = t.hutGrowth; e.poolGrowth = t.poolGrowth;
        e.progress = { s.total, s.quotas, s.used, s.next, s.lossRemainder }; return e;
    }
    struct Checked { bool valid = false; Resources cost{}; Evidence expected; std::string reason; };
    inline bool supportedBuilding( const uint32_t building ) { return building == BUILD_WEL2 || building == BUILD_TRAINING_GROUNDS; }
    inline Checked check( const Castle & town, const P::Town & projected, const P::Action & a, const Evidence & before )
    {
        Checked c; c.expected = before;
        if ( a.kind != P::Kind::Build || !supportedBuilding( a.building ) || a.count || a.pool || a.town != before.town || a.day != before.day
             || before.race != Race::KNGT || projected.race != before.race || projected.tile != before.town || projected.mask != before.buildings
             || !before.snapshotValid || !projected.incomeAttributionKnown ) { c.reason = "unsupported_build_shape_or_context"; return c; }
        if ( !town.AllowBuyBuilding( a.building ) ) { c.reason = "building_unavailable"; return c; }
        const auto found = projected.catalog.find( a.building );
        if ( found == projected.catalog.end() ) { c.reason = "building_catalog_missing"; return c; }
        const auto & config = Conscription::settings();
        auto price = found->second.price;
        if ( a.building == BUILD_TRAINING_GROUNDS ) price = { config.trainingGold, config.trainingWood, config.trainingOre, 0, 0, 0, 0 };
        else if ( before.buildings & BUILD_TRAINING_GROUNDS ) price = { config.farmGold, config.farmWood, config.farmOre, 0, 0, 0, 0 };
        c.cost = F::resources( town.getBuildingCost( a.building ) );
        Resources checked{};
        if ( c.cost != price || !F::checkedCost( c.cost, 1, before.money, checked ) ) { c.reason = "effective_cost_or_funds"; return c; }
        const uint32_t required = a.building == BUILD_TRAINING_GROUNDS ? config.requiredBuildings : found->second.prerequisites;
        if ( ( before.buildings & required ) != required ) { c.reason = "effective_prerequisites"; return c; }
        const uint32_t removed = a.building == BUILD_TRAINING_GROUNDS ? BUILD_WEL2 : BUILD_TRAINING_GROUNDS;
        c.expected.buildings = ( before.buildings & ~removed ) | a.building; c.expected.canBuild = false;
        c.expected.farm = a.building == BUILD_WEL2; c.expected.training = !c.expected.farm;
        c.expected.hutGrowth = !( before.buildings & DWELLING_MONSTER1 ) ? 0 : Monster( Monster::PEASANT ).GetGrown()
            + ( before.buildings & BUILD_WELL ? Castle::GetGrownWell() : 0 ) + ( c.expected.farm ? Castle::GetGrownWel2() : 0 );
        // One-town fixed-visitor fixture. The live income must reflect the changed
        // eligibility; construction must not credit today's income or train units.
        uint64_t peasants = 0;
        for ( const auto & army : { before.garrison, before.visitorArmy } )
            for ( const auto & stack : army ) if ( stack.first == Monster::PEASANT ) peasants += stack.second;
        const uint64_t eligible = !Kingdom::TaxpayerRequiresFarms() || c.expected.farm ? std::min<uint64_t>( INT32_MAX, peasants ) : 0;
        c.expected.tax = static_cast<int64_t>( std::min<uint64_t>( INT32_MAX, eligible * Kingdom::GetTaxpayerGoldPerPeasantPerDay() ) );
        const int64_t income = before.income[0] + c.expected.tax - before.tax;
        if ( income < 0 || income > INT32_MAX ) { c.reason = "income_range"; return c; }
        c.expected.income[0] = income;
        for ( size_t i = 0; i < 7; ++i ) c.expected.money[i] -= c.cost[i];
        c.valid = true; c.reason = "checked_build"; return c;
    }
    inline V::Outcome classify( bool returned, const Evidence & before, const Checked & checked, const std::optional<Evidence> & after )
    {
        if ( after && checked.valid && returned && *after == checked.expected ) return V::Outcome::Executed;
        if ( after && !returned && *after == before ) return V::Outcome::Failed;
        return V::Outcome::Uncertain;
    }
    struct Policy
    {
        using Evidence = BuildFixture::Evidence;
        using Checked = BuildFixture::Checked;
        using Call = std::function<bool( Castle &, uint32_t )>;
        static bool supports( const P::Action & a ) { return a.kind == P::Kind::Build && supportedBuilding( a.building ); }
        static Evidence capture( const Castle & t ) { return BuildFixture::capture( t ); }
        static Checked check( const Castle & t, const P::Town & p, const P::Action & a, const Evidence & e ) { return BuildFixture::check( t, p, a, e ); }
        static V::Outcome classify( bool r, const Evidence & b, const Checked & c, const std::optional<Evidence> & a ) { return BuildFixture::classify( r, b, c, a ); }
        static bool dispatch( Castle & t, const P::Action & a, const Checked &, const Call & callback ) { return callback ? callback( t, a.building ) : t.BuyBuilding( a.building ); }
    };
    using Owner = F::ActionOwner<Policy>;
}
