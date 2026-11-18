# Knight Mod Public Beta — work checkpoint

User authorized public x86/x64 beta on 2026-11-18. Base: v17 final checkpoint 9c94a1ac1380ffe971a0338a15611c7f3b616e9c; binary base 5eb08ed3b0ffd648ac48b4cddcd8bb3e0975024b.

Scope: public Release, no telemetry/debug/observer; retain v17 costs and AI; cap taxpayer.max_taxable_peasants_per_farm (0 unlimited); Farm eligibility mandatory and legacy taxpayer.require_farms ignored; all 23 current language catalogs isolated as km_*.mo; translate town/purchase text; safe batch uninstaller; English INI comments. README and test/balance guide explicitly deferred. New progressive TG/buffer system excluded.

Changes implemented: actual per-town cap and forecast cap; capped economic recruitment and fiscal route value; protected economic reserve bounded by cap. Existing eligibility of a stationed visitor is preserved so cap=0 reproduces v17. Public compile-time diagnostics false, trace payloads removed; km_ catalog loading and stdlib compiler with placeholders/encoding checks; translations in two TSV overlays. Reference INI provided as .default; active user config never overwritten. Uninstaller preserves active configuration, saves and original files; removes only exact public files and 23 language filenames.

Validation so far: native public game link succeeded (existing float-equal warning exempted from -Werror); public pure cap test passed; four native engine processes passed for 0/1/300/500, actual income matches forecast and old reveal/diagnostic switches remain ineffective. 26 v17 regression processes passed with public switches. All 23 catalogs generated and checked through GNUTranslations with placeholder and encoding round trips.

Native regression result records now contain 182 successful processes: v9 28, v13 64, v15 48, v16 16, v17 26. A later runner probe omitted --public for v15/v16 and failed its diagnostic-output expectation; this was a harness invocation failure, not an engine assertion. The successful JSON records remain authoritative. Public runners explicitly skip diagnostic expectations while retaining native outcome assertions. English-only prototype lock was removed from Settings::setGameLanguage, restoring actual km_ catalog selection.

Remaining: final native language test and incremental build; fresh Windows x86/x64 CI; artifact content/PE/profile/translation checks; deliver both packages. README and balancing guide deferred. Windows binaries cannot be executed on this Linux host; do not claim Windows 7 runtime validation from compilation alone.
