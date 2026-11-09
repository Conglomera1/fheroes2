#include "economy_registry_observation.h"
#include "economy_project_fixture.h"
#include <cassert>
#include <iostream>
namespace C = AI::EconomyCommitments;
namespace G = AI::EconomyCandidates;
namespace X = AI::EconomyMissions;
namespace V = AI::EconomyRegistry;
namespace H = AI::EconomyRegistryObservation;
namespace T = AI::EconomyStability;
G::Context context()
{
    G::Context c; c.base=fixture(false,1); c.base.throughDay=15;
    c.base.start.towns[0].hero=5; c.base.start.towns[0].visitor[0]={Monster::PEASANT,10};
    c.base.start.towns[0].visitor[1]={Monster::ARCHER,1}; assert(P::refresh(c.base.start)); return c;
}
X::Snapshot missions( const G::Context & c )
{ X::Hero hero; hero.id=5; hero.tile=hero.town=42; hero.role=X::Role::Fighter; hero.patrol=true; return X::classify(c.base.start,{hero},{},{}); }
C::Program program( P::Kind kind=P::Kind::ToVisitor, uint32_t count=3 )
{ C::Program p; p.id=10; p.town=42; p.visitor=5; p.allowAbandon=p.independent=true;
  auto a=action(7,1,kind,42,count); a.project=10; p.pending={a}; return p; }
T::Result selection( P::Action a )
{ a.id=a.project=0; T::Result s; s.certified=true; s.selected=0; s.search.plans.resize(1);
  s.search.plans[0].score.verified=true; s.search.plans[0].candidate.id=77; s.search.plans[0].candidate.actions={a}; return s; }
int main()
{
    size_t n=0; auto c=context(); const C::Stamp stamp{1,1,1}; auto m=missions(c); C::Book book{stamp,{program()}};
    auto out=C::import(c,m,stamp,book);
    assert(out.context && out.permissions.empty() && out.protectedProjects.count(10) && !out.fundingVerified); ++n;
    const auto gold=c.base.start.economy.gold; P::WorkBudget w;
    auto replay=P::project(out.context->base,w);
    assert(replay.covered() && P::count(replay.end.towns[0].garrison)==97 && P::count(replay.end.towns[0].visitor)==13
            && P::count(c.base.start.towns[0].garrison)==100 && P::count(c.base.start.towns[0].visitor)==10); ++n;
    // Isolate the transfer morning: no manual transfer cost or lost population.
    auto today=out.context->base; today.throughDay=1; P::WorkBudget wt; auto now=P::project(today,wt);
    assert(now.covered() && now.end.economy.gold==gold && P::count(now.end.towns[0].garrison)+P::count(now.end.towns[0].visitor)==110); ++n;
    book.programs={program(P::Kind::ToGarrison,10)}; out=C::import(c,m,stamp,book); P::WorkBudget wd;
    replay=P::project(out.context->base,wd);
    assert(replay.covered() && P::count(replay.end.towns[0].garrison)==110 && P::count(replay.end.towns[0].visitor)==0); ++n;
    auto last=c; last.base.start.towns[0].visitor[1]={}; assert(P::refresh(last.base.start));
    out=C::import(last,missions(last),stamp,book); P::WorkBudget wl;
    assert(out.context && P::project(out.context->base,wl).reason=="peasant_transfer_unavailable"); ++n;
    auto missing=book; missing.programs[0].visitor=-1;
    assert(C::import(c,m,stamp,missing).status==C::Status::Unsupported); ++n;
    auto wrong=book; wrong.programs[0].visitor=6;
    assert(C::import(c,m,stamp,wrong).reason=="local_transfer_visitor_not_fixed"); ++n;
    auto changed=c; changed.base.start.towns[0].hero=6;
    assert(!C::import(changed,m,stamp,book).context); ++n;
    auto mobile=m; mobile.heroes[0].patrolRadius=1; mobile.heroes[0].use=X::Use::MobilePatrol; mobile.stationaryVisitors.clear();
    assert(!C::import(c,mobile,stamp,book).context); ++n;
    auto fake=m; fake.heroes.clear(); assert(!C::import(c,fake,stamp,book).context); ++n;
    auto bad=book; bad.programs[0].pending[0].pool=1; assert(!C::import(c,m,stamp,bad).context); ++n;
    bad=book; bad.programs[0].pending[0].building=BUILD_WELL; assert(!C::import(c,m,stamp,bad).context); ++n;
    bad=book; bad.programs[0].pending[0].count=0; assert(!C::import(c,m,stamp,bad).context); ++n;
    auto insufficient=book; insufficient.programs[0].pending[0].count=11; out=C::import(c,m,stamp,insufficient); P::WorkBudget wi;
    assert(out.context && P::project(out.context->base,wi).reason=="peasant_transfer_unavailable"); ++n;
    auto full=c; full.base.start.towns[0].visitor={{{Monster::ARCHER,1},{Monster::PIKEMAN,1},{Monster::SWORDSMAN,1},{Monster::CAVALRY,1},{Monster::PALADIN,1}}};
    assert(P::refresh(full.base.start)); C::Book withdrawal{stamp,{program()}}; out=C::import(full,missions(full),stamp,withdrawal); P::WorkBudget ws;
    assert(out.context && P::project(out.context->base,ws).reason=="peasant_transfer_unavailable"); ++n;
    // Actual source plan -> snapshot registry preserves visitor identity, count and direction.
    auto observed=H::stage(c,m,selection(program().pending[0]),1,100);
    assert(observed.status==H::Status::Staged && observed.registry && observed.registry->boundVisitor(1)==5
            && observed.registry->view().context->base.actions[0].kind==P::Kind::ToVisitor); ++n;
    const auto receipt=*observed.registry->ticket(1); auto altered=receipt; altered.visitor=6;
    assert(receipt.visitor==5 && observed.registry->record(altered,V::Outcome::Executed).status==V::Status::Invalid && observed.registry->pending()==1); ++n;
    assert(observed.registry->refresh(changed,m).status==V::Status::Held && !observed.registry->ticket(1) && !observed.registry->view().context); ++n;
    // Conscription still uses the real scheduler and effective fees after withdrawal.
    auto training=c; auto & town=training.base.start.towns[0]; town.mask &= ~BUILD_WEL2; town.mask |= BUILD_TRAINING_GROUNDS;
    training.base.config.oddDayLimit=training.base.config.evenDayLimit=1; training.base.config.goldPerPeasant=137;
    training.base.start.economy.nonTaxIncome=training.base.start.economy.taxRate=0; assert(P::refresh(training.base.start)); training.base.throughDay=2;
    auto all=program(P::Kind::ToVisitor,100); C::Book allBook{stamp,{all}}; auto imported=C::import(training,missions(training),stamp,allBook); P::WorkBudget wa;
    auto trained=P::project(imported.context->base,wa);
    assert(trained.covered() && trained.end.economy.gold==training.base.start.economy.gold-157
            && P::count(trained.end.towns[0].garrison)==0 && P::count(trained.end.towns[0].visitor)==110); ++n;
    auto unbound=H::stage(c,mobile,selection(program().pending[0]),1,101); assert(!unbound.registry); ++n;
    auto remote=book; remote.programs[0].destination=C::Destination::RemoteHero; remote.programs[0].hero=5;
    assert(C::import(c,m,stamp,remote).reason=="remote_delivery_not_implemented"); ++n;
    std::cout<<"PASS "<<n<<" fixed-local-transfer oracles (actor identity, both directions, last troop, slots, actual INI scheduler and registry binding)\n";
}
