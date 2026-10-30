# Clean Taxpayer milestone

Gameplay baseline: 8a89d86. This branch preserves the validated Taxpayer AI,
quick-payback Farm construction and default preservation of taxpayers.
It is a player milestone, not the renamed 1.0b distribution.

All milestone builds omit FHEROES2_TAXPAYER_TEST and
FHEROES2_TAXPAYER_DIAGNOSTICS. Windows explicitly disables both MSBuild
properties. Existing fheroes2.cfg test settings cannot enable observer powers
or Taxpayer telemetry in these binaries. Normal engine errors/logging remain.

Targets: Windows x86/x64 ZIPs and installers, Linux x86-64/ARM64,
macOS Intel ZIP and ARM64 app bundle, Android Release APK/AAB,
PS Vita, Nintendo Switch, WebAssembly single/multi-threaded, and an iOS
Release simulator validation build. iOS is not a signed device distribution.
Android Release is signed using the development signing configuration; it is
not a production/store signature and update compatibility is not guaranteed.
Original Heroes II assets are not included. Packages include engine assets
and translations as supported by each existing platform workflow.

Only build artifacts are uploaded; no release, store upload, or website
publication is performed. Compilation does not establish runtime validation.
Before using this as 1.0b, check startup, taxpayer configuration, income UI,
Farm descriptions and save/load on the actual target platforms.
