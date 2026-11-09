#include "economy_registry_observation.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace H = AI::EconomyRegistryObservation;
namespace G = AI::EconomyCandidates;
namespace T = AI::EconomyStability;
namespace X = AI::EconomyMissions;
G::Context context()
{ G::Context c; c.base = fixture(false,1); c.base.throughDay=15; c.base.start.economy.towns[0].troops.pools[0]=50; return c; }
T::Result selection( std::vector<P::Action> actions )
{
    T::Result s; s.certified=true; s.selected=0; s.search.plans.resize(1);
    s.search.plans[0].score.verified=true; s.search.plans[0].candidate.id=77; s.search.plans[0].candidate.actions=std::move(actions); return s;
}
int main()
{
    size_t n=0; auto c=context(); std::optional<X::Snapshot> m=X::classify(c.base.start,{},{},{});
    auto a=G::recruit(42,1,1,1); auto s=selection({a});
    auto out=H::stage(c,m,s,1,100);
    assert(out.status==H::Status::Staged && out.registry && out.registry->pending()==1 && out.programs==1 && !out.actionable); ++n;
    const auto imported=out.registry->view();
    assert(imported.context && imported.permissions.empty() && imported.protectedProjects.count(1) && !imported.fundingVerified && c.base.actions.empty()); ++n;
    assert(imported.context->base.start.economy.gold==c.base.start.economy.gold && imported.context->base.start.towns[0].garrison[0].count==100); ++n;
    assert(out.links.size()==1 && out.links[0].sourceAction==1 && out.links[0].sourceProject==77 && out.links[0].action==1 && out.links[0].sourceIndex==0); ++n;
    P::Input before; assert(G::materialize(c,s.search.plans[0].candidate,before)); P::WorkBudget wb,wa;
    const auto original=P::project(before,wb), registered=P::project(imported.context->base,wa);
    assert(original.covered() && registered.covered() && original.end.economy.gold==registered.end.economy.gold && original.purchases.size()==registered.purchases.size()); ++n;
    auto empty=selection({}); auto noOp=H::stage(c,m,empty,1,101);
    assert(noOp.status==H::Status::Empty && noOp.registry && noOp.registry->pending()==0 && noOp.links.empty() && noOp.programs==0); ++n;
    auto unverified=selection({a}); unverified.certified=false;
    assert(!H::stage(c,m,unverified,1,102).registry); ++n;
    unverified=selection({a}); unverified.search.plans[0].score.verified=false;
    assert(!H::stage(c,m,unverified,1,102).registry); ++n;
    unverified=selection({a}); unverified.selected=2;
    assert(!H::stage(c,m,unverified,1,102).registry); ++n;
    assert(!H::stage(c,{},s,1,102).registry && !H::stage(c,m,s,1,0).registry && !H::stage(c,m,s,0,102).registry); ++n;
    auto missing=m; missing->gaps.insert("unknown_assignment"); assert(!H::stage(c,missing,s,1,102).registry); ++n;
    auto active=c; active.active={a}; assert(H::stage(active,m,s,1,102).status==H::Status::Unsupported); ++n;
    auto retained=c; auto existing=a; existing.id=existing.project=3; retained.base.actions={existing};
    assert(H::stage(retained,m,s,1,102).status==H::Status::Unsupported); ++n;
    auto transfer=a; transfer.kind=P::Kind::ToGarrison; auto unsupported=selection({a,transfer});
    out=H::stage(c,m,unsupported,1,102); assert(out.status==H::Status::Unsupported && !out.registry && out.links.empty()); ++n;
    auto many=selection(std::vector<P::Action>(1025,a)); assert(H::stage(c,m,many,1,102).status==H::Status::Limit); ++n;
    auto bad=a; bad.town=999; assert(!H::stage(c,m,selection({bad}),1,102).registry); ++n;
    bad=a; bad.day=0; assert(!H::stage(c,m,selection({bad}),1,102).registry); ++n;
    // Grouping preserves chronological order and source links across two towns.
    auto other=c.base.start.towns[0]; other.tile=70; c.base.start.towns.push_back(other);
    auto economic=c.base.start.economy.towns[0]; economic.tile=70; c.base.start.economy.towns.push_back(economic);
    m=X::classify(c.base.start,{},{},{}); auto b=G::recruit(70,1,1,1); auto a2=a; a2.day=2;
    out=H::stage(c,m,selection({a,b,a2}),1,103);
    assert(out.status==H::Status::Staged && out.programs==2 && out.links.size()==3 && out.registry->view().permissions.empty()); ++n;
    assert(out.links[0].sourceIndex==0 && out.links[1].sourceIndex==1 && out.links[2].sourceIndex==2 && out.links[2].action==2); ++n;
    auto b2=b; b2.day=2;
    out=H::stage(c,m,selection({a,b,b2,a2}),1,104);
    assert(out.status==H::Status::Unsupported && out.reason=="registry_grouping_would_reorder_schedule" && !out.registry && out.links.empty()); ++n;
    out=H::stage(c,m,selection({a,b,a}),1,105);
    assert(out.status==H::Status::Unsupported && !out.registry); ++n;
    auto dated=selection({a2,a}); out=H::stage(c,m,dated,1,106);
    assert(out.registry && out.links[0].sourceIndex==1 && out.links[1].sourceIndex==0); ++n;
    auto required=a; required.required=true;
    out=H::stage(c,m,selection({required}),1,107);
    assert(out.registry && out.registry->view().context->base.actions[0].required); ++n;
    std::cout<<"PASS "<<n<<" snapshot-registry bridge oracles (selection gate, source links, exact calendar order, protection and no world mutation)\n";
}
