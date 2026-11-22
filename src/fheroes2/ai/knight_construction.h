// Shared funding, bounded city roles and physical population transfers.
#pragma once
#include <array>
#include <cstddef>
#include <utility>
#include <cstdint>
#include <map>
#include <vector>
class Castle;
class Kingdom;
struct Funds;
class Heroes;
namespace AI::KnightConstruction
{
    using Resources = std::array<int64_t, 7>;
    struct Promise { uint32_t building = 0; size_t priority = 0, order = 0; Resources cost{}; };
    inline bool covers( const Resources & funds, const Resources & cost )
    {
        for ( size_t i = 0; i < funds.size(); ++i ) if ( funds[i] < cost[i] ) return false;
        return true;
    }
    class Ledger
    {
    public:
        Resources reserved( const int town = -1, const uint32_t building = 0 ) const
        {
            Resources sum{};
            for ( const auto & [tile, p] : promises ) {
                if ( tile == town && p.building == building ) continue;
                for ( size_t i = 0; i < sum.size(); ++i ) sum[i] += p.cost[i];
            }
            return sum;
        }
        bool book( const int town, Promise p, const Resources & tomorrow )
        {
            auto need = reserved();
            if ( promises.count( town ) ) return false;
            for ( size_t i = 0; i < need.size(); ++i ) {
                if ( p.cost[i] < 0 || need[i] > tomorrow[i] || p.cost[i] > tomorrow[i] - need[i] ) return false;
            }
            promises.emplace( town, p ); return true;
        }
        bool allows( const int town, const uint32_t building, const Resources & after ) const
        { return covers( after, reserved( town, building ) ); }
        void release( const int town, const uint32_t building )
        { const auto p = promises.find( town ); if ( p != promises.end() && p->second.building == building ) promises.erase( p ); }
        std::vector<int> reconcile( const Resources & tomorrow )
        {
            std::vector<int> dropped;
            while ( !covers( tomorrow, reserved() ) && !promises.empty() ) {
                auto worst = promises.begin();
                for ( auto it = promises.begin(); it != promises.end(); ++it )
                    if ( std::make_pair( it->second.priority, it->second.order ) > std::make_pair( worst->second.priority, worst->second.order ) ) worst = it;
                dropped.push_back( worst->first ); promises.erase( worst );
            }
            return dropped;
        }
        std::map<int, Promise> promises;
    };
    // Active throughout the actual AI turn, including hero visits and purchases.
    class Scope
    {
    public:
        explicit Scope( const Kingdom & kingdom, const std::vector<const Castle *> & order = {} );
        ~Scope();
        Scope( const Scope & ) = delete;
        Scope & operator=( const Scope & ) = delete;
    private:
        const Kingdom * kingdom_;
    };
    bool canSpend( const Castle & castle, uint32_t building );
    // Shared live budget, used before routine recruitment, upgrades and exchanges.
    Funds spendable( const Kingdom & kingdom );
    bool allowsPayment( const Kingdom & kingdom, const Funds & cost, const char * purpose );
    bool canTradeForBuilding( const Castle & castle, uint32_t building, const Funds & transaction );
    Resources reservedDevelopment( const Kingdom & kingdom );
    int64_t operatingReserve( const Kingdom & kingdom );
    bool mobilizationPreservesDevelopment( const Castle & castle );
    bool hasPrimaryChoice( const Castle & castle );
    bool economicTown( const Castle & castle );
    bool populationSafe( const Castle & castle );
    uint32_t populationReserve( const Castle & castle );
    bool canReceivePopulation( const Heroes & hero, const Castle & castle );
    bool canLoadPopulation( const Heroes & hero, const Castle & castle, uint32_t quantity );
    uint32_t populationSupplyNeed( const Castle & castle, int exceptHero = -1 );
    uint32_t populationAvailableStock( const Castle & castle, int exceptHero = -1 );
    int populationPickupDestination( const Heroes & hero, const Castle & source );
    void preparePopulation( Kingdom & kingdom );
    int populationDestination( const Heroes & hero );
    int populationOrder( const Heroes & hero );
    void cancelPopulationRoute( const Heroes & hero, const char * reason );
    bool keepPopulationCargo( const Heroes & hero );
    uint32_t cancelledCargo( const Heroes & hero );
    void resolveCancelledCargo( const Heroes & hero );
    void settleCancelledCargo( Heroes & hero, Castle & castle );
    bool populationDepositProtected( const Castle & castle );
    void protectPopulationDeposit( const Castle & castle );
    void notePopulationRoute( const Heroes & hero, int destination );
    void arrangePopulation( Heroes & hero, Castle & castle, int destination = -1, bool embarking = false );
    // Permit the producing town's infantry purchases to use its own envelope.
    class Recruitment
    {
    public:
        explicit Recruitment( const Castle & castle );
        ~Recruitment();
    private:
        const Castle * previous_;
    };
    void setRoles( const Kingdom & kingdom, const std::vector<int> & knownFront, const std::vector<int> & endangered );
    void refresh( bool beforeTrainingOrders = false );
    bool active( const Kingdom & kingdom );
    class Emergency
    {
    public:
        explicit Emergency( bool active );
        ~Emergency();
    private:
        bool active_;
    };
    int navalTarget( const Heroes & hero, bool renew = false );
    void noteLandObjective( const Heroes & hero, int target );
    // Called before movement (also after a new hero is hired). Return changed
    // map tiles so the planner can refresh its native object/pathfinding caches.
    std::vector<int> prepareNaval( Kingdom & kingdom );
    // Only the first exploration voyage launched from a port during week one.
    Heroes * openingRecruit( const Castle & port, Heroes * first, Heroes * second );
    void noteOpeningRecruit( const Heroes & hero, const Castle & port );
    void cancelOpeningPreparation( const Castle & port );
    bool prepareOpeningExplorer( Heroes & hero, Castle & port, int target = -1 );
    bool prepareOpeningParty( Heroes & land, Heroes & sailor, Castle & port );
    bool keepOpeningLandPeasants( const Heroes & hero );
    void settleOpeningTaxpayers( Heroes & land, Castle & port, bool neededInArmy );
    bool isOpeningSeaExplorer( const Heroes & hero );
    void markOpeningEmbarked( const Heroes & hero );
}
