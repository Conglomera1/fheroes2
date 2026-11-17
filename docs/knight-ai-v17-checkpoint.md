# Knight AI v17 — finalisation en cours

Base exacte : c85ca31800a43d3058b56f5ea8e6c4270f81678f, production v16 66945761c21850f8cf6008a0a9b731cc768bb7a1.
Autorisation utilisateur : finaliser les deux cas de cargaison/ville reconvertie, tests ciblés puis paquet normal. Coûts, priorités de construction, ouverture J1 et navigation après J1 inchangés. Pas de nouvel autoplay utilisateur requis.

Code : cargaison annulée mémorisée et restituée avant renforcement ; dépôt protégé pendant le tour du royaume. Changements de rôle TG→Farm annulent une collecte non exécutée ou réévaluent le dépôt chargé. Nettoyage au renvoi du héros. Réglages de release distincts des anciennes clés de debug, observation et diagnostics désactivés par défaut et réactivables.

État réel : compilation native stricte finale réussie. 189 exécutions réussies : 26 v17, 156 régressions v16/v15/v13/v9 et 7 réglages release. Diagnostics 0/2 : résultats identiques et transferts conservateurs. Paramètres normaux par défaut vérifiés, y compris présence des anciennes clés d’observer. Résultats complets dans knight-ai-v17-validation.json.

Reste : compiler et vérifier le paquet Windows x86 normal via km-release.yml, puis sauvegarder ZIP et bilan. Ne pas relancer les tests natifs terminés sauf nouvelle modification de code. Ne pas distribuer avant vérification du paquet.
