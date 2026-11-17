// Compile this fixture with KNIGHT_RELEASE and exclude settings.o.
#include "../src/fheroes2/system/settings.cpp"
#include <cassert>
#include <iostream>
int main(int argc,char ** argv) {
 assert(argc==3);const std::string mode=argv[1];auto & s=Settings::Get();assert(s.Read(argv[2]));
 const bool observer=mode=="observer"||mode=="both";
 const int level=mode=="diagnostics"||mode=="both"?1:mode=="clamped"?2:0;
 assert(s.isKnightObserverBuild()==observer&&s.isTaxpayerTestMapRevealEnabled()==observer);
 assert(s.taxpayerTestDiagnostics()==level&&s.taxpayerTraceDetail()==(observer&&level?2:level));
 std::cout<<"PASS release_settings "<<mode<<" observer="<<observer<<" diagnostics="<<level<<'\n';
}
