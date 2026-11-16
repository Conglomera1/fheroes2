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
