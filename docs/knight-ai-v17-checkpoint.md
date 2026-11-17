# Knight AI v17 — finalisation en cours

Base exacte : c85ca31800a43d3058b56f5ea8e6c4270f81678f, production v16 66945761c21850f8cf6008a0a9b731cc768bb7a1.
Autorisation utilisateur : finaliser les deux cas de cargaison/ville reconvertie, tests ciblés puis paquet normal. Coûts, priorités de construction, ouverture J1 et navigation après J1 inchangés. Pas de nouvel autoplay utilisateur requis.

Code : cargaison annulée mémorisée et restituée avant renforcement ; dépôt protégé pendant le tour du royaume. Changements de rôle TG→Farm annulent une collecte non exécutée ou réévaluent le dépôt chargé. Nettoyage au renvoi du héros. Réglages de release distincts des anciennes clés de debug, observation et diagnostics désactivés par défaut et réactivables.

État réel : première compilation native réussie ; compilation après réglages de release en cours. Les premiers scénarios de cargaison passent ; le fixture de déplacement doit être relancé après correction de son assertion de stock, qui ignorait le recrutement natif des Paysans de Hut. NE PAS annoncer les suites finales réussies à ce point.

À terminer : tests v17 et réglages, régressions v16/v15/v13/v9 sur les objets finaux, CI Windows x86 normale via km-release.yml, vérification de l’archive puis paquet/bilan sauvegardés. Ne pas distribuer un candidat non vérifié.
