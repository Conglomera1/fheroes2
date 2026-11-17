# Knight AI v17 — finalisation terminée

Base exacte : c85ca31800a43d3058b56f5ea8e6c4270f81678f, production v16 66945761c21850f8cf6008a0a9b731cc768bb7a1.
Autorisation utilisateur : finaliser les deux cas de cargaison/ville reconvertie, tests ciblés puis paquet normal. Coûts, priorités de construction, ouverture J1 et navigation après J1 inchangés. Pas de nouvel autoplay utilisateur requis.

Code : cargaison annulée mémorisée et restituée avant renforcement ; dépôt protégé pendant le tour du royaume. Changements de rôle TG→Farm annulent une collecte non exécutée ou réévaluent le dépôt chargé. Nettoyage au renvoi du héros. Réglages de release distincts des anciennes clés de debug, observation et diagnostics désactivés par défaut et réactivables.

État réel : compilation native stricte finale réussie. 189 exécutions réussies : 26 v17, 156 régressions v16/v15/v13/v9 et 7 réglages release. Diagnostics 0/2 : résultats identiques et transferts conservateurs. Paramètres normaux par défaut vérifiés, y compris présence des anciennes clés d’observer. Résultats complets dans knight-ai-v17-validation.json.

Livraison terminée. Source exacte du binaire : 5eb08ed3b0ffd648ac48b4cddcd8bb3e0975024b (arbre 998014926be597bebc339c078e562878f1fa7f1d).
Compilation Windows normale réussie : https://github.com/Conglomera1/fheroes2/actions/runs/44152144617
Paquet contrôlé : CRC des deux archives, SHA-256 de l’artefact GitHub, PE x86, commit embarqué, révision 17, 35 marqueurs et identité des ressources/README. Aucun lancement Windows dans l’environnement Linux. Détails dans knight-ai-v17-package-verification.json.

Knight-Mod-v17-Windows-x86.zip : https://chatgpt.com/library?file_id=libfile_4884c48ec6908191b4e3f455152e5907&account_id=personal
Knight_Mod_v17_Bilan.md : https://chatgpt.com/library?file_id=libfile_306aece5f0cc8191babffe91ba11a5b2&account_id=personal

Lancer fheroes2KM.exe dans une installation existante avec les données Heroes II et SDL 32 bits. Observer et télémétrie désactivés par défaut ; réactivables via knight observer et knight diagnostics dans fheroes2.cfg. INI d’équilibrage conservé.

Aucune étape restante pour cette livraison. Ne pas reconstruire ni relancer les 189 exécutions terminées sans changement ou défaut nouveau. Aucun nouvel autoplay long demandé. La branche release reste sur le commit source validé ; cette mise à jour documentaire est publiée uniquement sur la branche checkpoints pour éviter une compilation superflue.
