# Knight Mod Public Beta — work checkpoint

User authorized public x86/x64 beta on 2026-11-18. Base: v17 final checkpoint 9c94a1ac1380ffe971a0338a15611c7f3b616e9c; binary base 5eb08ed3b0ffd648ac48b4cddcd8bb3e0975024b.

Scope: public Release, no telemetry/debug/observer; retain v17 costs and AI; cap taxpayer.max_taxable_peasants_per_farm (0 unlimited); Farm eligibility mandatory and legacy taxpayer.require_farms ignored; all 23 current language catalogs isolated as km_*.mo; translate town/purchase text; safe batch uninstaller; English INI comments. README and test/balance guide explicitly deferred. New progressive TG/buffer system excluded.

Changes implemented: actual per-town cap and forecast cap; capped economic recruitment and fiscal route value; protected economic reserve bounded by cap. Existing eligibility of a stationed visitor is preserved so cap=0 reproduces v17. Public compile-time diagnostics false, trace payloads removed; km_ catalog loading and stdlib compiler with placeholders/encoding checks; translations in two TSV overlays. Reference INI provided as .default; active user config never overwritten. Uninstaller preserves active configuration, saves and original files; removes only exact public files and 23 language filenames.

Validation so far: native public game link succeeded (existing float-equal warning exempted from -Werror); public pure cap test passed; four native engine processes passed for 0/1/300/500, actual income matches forecast and old reveal/diagnostic switches remain ineffective. 26 v17 regression processes passed with public switches. All 23 catalogs generated and checked through GNUTranslations with placeholder and encoding round trips.

Native regression result records now contain 182 successful processes: v9 28, v13 64, v15 48, v16 16, v17 26. A later runner probe omitted --public for v15/v16 and failed its diagnostic-output expectation; this was a harness invocation failure, not an engine assertion. The successful JSON records remain authoritative. Public runners explicitly skip diagnostic expectations while retaining native outcome assertions. English-only prototype lock was removed from Settings::setGameLanguage, restoring actual km_ catalog selection.

Remaining: final native language test and incremental build; fresh Windows x86/x64 CI; artifact content/PE/profile/translation checks; deliver both packages. README and balancing guide deferred. Windows binaries cannot be executed on this Linux host; do not claim Windows 7 runtime validation from compilation alone.

## Public candidate source

Published source: 9b18203b82136d4b2e632725d3b8d3ba29dabfc8 (tree 6ce8b87faf2c4aa499a82a1bab3588d7a4e224c2). Windows CI run 44209481865: both compilers are running. Earlier Windows runs compiled both architectures with zero warnings/errors but failed the Python overlay step because Windows defaulted to CP1252. Explicit UTF-8 decoding fixed this; the script now also passes with Python UTF-8 mode disabled and a C locale.

Native final link passed. 23 catalogs passed the actual C++ Translation loader, TG names, purchase strings, Farm text and daily conscription description; placeholders fully substituted. Fresh active INI generation contains all 32 settings and matches the reference exactly. Static uninstaller check confirms exact owned files and 23 exact catalog names only, preserves originals, active config and saves. The native binary has no Knight telemetry markers and contains the public caption. Short string literals can be split by optimization: do not use km_ or a full H2D filename as mandatory raw binary markers; verify catalog behavior and package hashes instead.

All game source is unchanged since 0737de7e040f42786a9307b4ec3d6b6e1abaaec0; subsequent commits complete the reference INI, add a test, fix binary checks, and correct the Windows packaging script. Validation-only updates belong on this separate branch to avoid triggering another Windows compilation.

## Final delivery checkpoint

Release source f9cc953a126dc3526d1b902e48fb8d6b7b762df4, tree a075314438424ec86dccab1f34e06ca443d8fdd7. CI https://github.com/Conglomera1/fheroes2/actions/runs/44210387725 succeeded for x86 and x64. Both builds have zero warnings and zero errors. Windows packaging caught a retained uncalled trace emitter in the optimized executable; the final commit explicitly preprocessor-excludes ensureTraceSegment emission and all legacy raw COUT diagnostic emitters. Only diagnostics were changed; the trace segment identity still used by AI is preserved.

After that final change: native link passed; 90 v15/v16/v17 fixture processes passed again (48/16/26); four cap engine processes passed again for 0/1/300/500 with old debug/observer/reveal requests ignored. Earlier 92 v9/v13 processes remain successful. Native catalog test passed for all 23 catalogs and generated active INI matches all 32 reference settings.

Both downloaded ZIPs passed tests/check_knight_public_package.py: exact 32-file allowlist; correct GUI PE architecture; exact source commit; 23 deterministic catalogs matching local compiler output; H2D and PNG hashes matching source; expected public caption/cap marker and absent Knight telemetry payloads; no overwrite of base executable/config/catalogs/DLLs. Batch uninstaller has exact mod file and language lists, preserves originals/config/saves; it has not been executed on Windows. The executables have not been run on this Linux host. Full validation metadata, architecture and SHA256 hashes are in knight-public-beta-validation.json. No README or balancing guide was produced.

Deliverables: Knight-Mod-Public-Beta1-Windows-x86.zip and Knight-Mod-Public-Beta1-Windows-x64.zip. Update packages require an existing compatible fheroes2 SDL2 installation of the matching architecture; launch fheroes2KM.exe. Cap defaults to 0 (unlimited); Farm taxation is mandatory; all v17 economic/conscription costs retained. New weekly buffer/progressive conscription is out of scope.
