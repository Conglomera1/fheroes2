// Test-only drawing substitute; does not replace planning, movement or world state.
extern "C" void __wrap__ZNK9Interface8GameArea6RedrawERN8fheroes25ImageEib(const void *, void *, int, bool) {}
extern "C" void __wrap__ZNK9Interface11StatusPanel7_redrawEv(const void *) {}
// No desktop input/quit event exists in this headless fixture.
extern "C" bool __wrap__ZN10LocalEvent12HandleEventsEbb(void *, bool, bool) { return true; }
