// One-turn construction funding. No saved orders and no gameplay mutations.
#pragma once
#include <array>
#include <cstddef>
#include <utility>
#include <cstdint>
#include <map>
#include <vector>
class Castle;
class Kingdom;
class Funds;
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
    bool mobilizationPreservesDevelopment( const Castle & castle );
    bool economicTown( const Castle & castle );
    void setRoles( const Kingdom & kingdom, const std::vector<int> & knownFront, const std::vector<int> & endangered );
    void refresh();
    bool active( const Kingdom & kingdom );
    class Emergency
    {
    public:
        explicit Emergency( bool active );
        ~Emergency();
    private:
        bool active_;
    };
    int navalTarget( const Heroes & hero );
}
