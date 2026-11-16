# Knight AI v15 — état de reprise

Base publiée : bd55a1d3f8ba5f884f8c9ea8e49b2345b2257e1e (v14).
Autorisation utilisateur : poursuivre les trois corrections, avec sauvegardes fréquentes (quota hebdomadaire ~5 %).

## Preuves conservées

Audit : Audit_v14_AoW_Utopeia.md, Library libfile_b697d87a4d8881919dcd2046ffde7b66.
Logs : logs8.rar, Library libfile_2d886c82c0188191ac3cd9cf888f2bee.
AoW : 721 événements, aucun NAVAL_OPENING_J1/PARTY, cinq navigateurs J1, surplus laissé au château.
Utopeia : 161707 événements J1–179, 315/315 conscriptions, zéro plan de collecte anticipée.
Hilltop TG J22 puis Farm J29, 17 recrues pour 4000 or et 10 bois/10 minerai de conversions.
Alamar : 573 Paysans Troy→Avalon J158, destination occupée J159, retour Troy J161 puis 577 Paysans livrés Avalon J163.

## Périmètre autorisé

1. Préparation/recrutement terrestre → sortie d'une case → recrutement navigateur/partage → dégagement terrestre → embarquement. Vérifier dans le tour IA complet, avec véritable côte et passage de château.
2. Destination fiscale amie temporairement occupée : éviter un grand détour immédiat, garder les urgences et replis sûrs.
3. Réexaminer l'intérêt économique des conversions temporaires courtes, coûts réels INI compris.
Ne pas modifier navigation après J1, coûts par défaut ni crash entre autoplays.

## Point de sauvegarde 1

Audit terminé ; aucune modification de production v15 à ce point. Ne pas reprendre les compilations/tests v14 : terminés et livrés.
Prochaine étape : reproduire le refus de préparation J1 et tracer ses gardes précoces avant correction.

## Point de sauvegarde 2 (en cours, non livrable)

Modifications de production : cases COAST admises à J1, prévision d'un pas de dégagement avant achat et dégagement payé après partage ; motifs de refus et événement de dégagement. Destination occupée : conserver la mission un jour avec position d'attente adjacente, puis repli si l'occupation persiste ; cargaison protégée en attente. Cycle temporaire : exiger aussi une trésorerie projetée au moins égale à conserver la Farm (coûts réels et recrutements inclus), ce qui exclut le cas Hilltop observé.
Compilation native v15 réussie. Première compilation du nouveau fixture corrigée (namespace Economy).
Tests v15 EN COURS : ne pas annoncer validés, ne pas livrer de binaire à ce point. Ajouter/faire passer le tour complet et les régressions concernées avant publication finale.
Chemins scratch (reconstructibles depuis code) : native-check/v15-build-final.log, v15-fixture-build.log, v15-tests ; tests/run_knight_v15.py.

## Point de sauvegarde 3

46 exécutions v15 passées (23 scénarios x diagnostics 0/2), résultats identiques et effectifs conservés. Inclut côte, attente puis libération, attente expirée et danger ; AoW/CoX et livraison TG pleine hérités v14. 28 régressions économiques v9 passées avec la logique v15.
IMPORTANT : aucun .gch ne doit rester à côté de ai_planner_castle.cpp pendant la compilation des fixtures (GCC peut utiliser le contenu précompilé antérieur). Celui de v14 a été déplacé hors source.
Un dernier ajustement donne aussi priorité aux objectifs militaires urgents accessibles pendant l'attente : compilation native finale en cours/réussie selon v15-build-final2.log, tests à relier sur ces objets.
Test opening_full en préparation : appeler KingdomTurn avant le Scope de fixture ; wrappers de lien limités à drawAITurnProgress et generateScrollbarSlider pour fonctionner sans assets d'interface. Les précédents essais ont bloqué dans l'UI puis sur un Scope doublé du fixture. NE PAS annoncer ce test réussi avant PASS opening_full dans v15-full-turn.log.
Le script tests/run_knight_v15.py n'inclut pas encore opening_full. Le binaire v15-full-tests utilise --ldflags='-Wl,--wrap=_ZN9Interface11StatusPanel18drawAITurnProgressEj -Wl,--wrap=_ZN8fheroes223generateScrollbarSliderERKNS_5ImageEbiiiRKNS_10RectBase2DIiiEES6_'.
Encore à faire : tour complet, validation finale après dernier ajustement, version/revision15 et README, Windows CI et paquet/bilan. La v14 distribuée reste intacte.

## Point de sauvegarde 4 — candidat Windows

64 régressions v13 passent également, soit 138 exécutions réussies avec les 46 v15 et 28 économiques. Le tour complet est toujours en validation (ne pas le compter avant PASS).
La configuration du fixture complet nécessite aussi les drapeaux natifs de joueurs en partie et une identité humaine pour les helpers d'affichage/alliance ; l'IA testée reste bleue et contrôlée par IA. Le mode autoplay et GameOver::Reset sont initialisés. Les wrappers incluent désormais le dessin de carte/statut et la pompe d'événements de bureau sans événement de quitter.
Revision de journal 15 et README mis à jour. Coûts, INI et navigation après J1 inchangés.
Le candidat Windows peut compiler en parallèle de la fin de validation ; ne pas le distribuer avant le résultat du tour complet et de la suite finale.

## Point de sauvegarde 5 — tour complet passé

PASS v15 opening_full day=1 gold=2750 constaté : appel natif KingdomTurn, partage 70 Paysans/6 Archers au terrestre, cinq singles au navigateur, dégagement 748→749 puis embarquement. L'état de joueurs en partie a été initialisé dans le fixture. Les wrappers sont uniquement UI/événements de bureau.
Suite finale de 24 cas x diagnostics 0/2 lancée dans native-check/v15-final-results (48 exécutions attendues) ; le script vérifie aussi seq(PARTY) < seq(CLEARANCE) < seq(embarquement) dans le tour complet. Ne pas compter la suite finale avant sa ligne PASS.
Les 64 régressions population et 28 économiques restent acquises. Compilation native release v15 réussie (v15-build-release.log). Publication Windows autorisée en parallèle ; vérifier archive et terminer bilan après PASS final.

## Point de sauvegarde 6 — validation finale conservée

PASS 48 v15 processes, diagnostics neutral confirmé. Avec 64 régressions v13 et 28 économiques v9 : 140 exécutions réussies. Résultats détaillés conservés dans docs/knight-ai-v15-validation.json pour résister à une réinitialisation du scratch.
Le tour complet passe diagnostics 0 et 2. Correction du vérificateur Python : le binaire natif ne compile pas les hooks Windows AI_ACTION_COMPLETE ; l'embarquement est donc vérifié par l'état réel isShipMaster après KingdomTurn (assertion du fixture), et la séquence structurée vérifie PARTY avant CLEARANCE. Aucun changement de production après c2a4a7378c1a2055676c61231cfbff38c86ff1db, seul le vérificateur et la documentation ont changé.
Compilation Windows en cours : run 43955201497, source c2a4a7378c1a2055676c61231cfbff38c86ff1db. Ne pas relancer sans échec établi. Après succès : récupérer artefact, contrôler digest/ZIP/PE/commit/marqueurs, produire KM-Observer-Debug-v15-Windows-x86.zip et Knight_AI_v15_Bilan.md, puis les sauvegarder ensemble.
