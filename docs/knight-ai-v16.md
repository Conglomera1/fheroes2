# Knight AI v16 — ravitaillement TG effectif

Base exacte : checkpoint v15 5643f3df8532c4472b907130ddc130ee6e64d0e5 ; production c2a4a7378c1a2055676c61231cfbff38c86ff1db.
Reprise autorisée du ravitaillement concret. Les discussions d'équilibrage suivantes restent en attente.

## Reproduction

logs9.rar : aucun POPULATION_SUPPLY_PLAN sur AoW J1–209 ni Utopeia J1–178. 14 conscriptions manquées à Blackfang, 54 à Westfork. Le correctif ne prétend pas les supprimer toutes.
Cas Utopeia J143 : Westfork 5678, TG sans Paysan en garnison, Hut 7 ; Timberhill 5841, Farm, 283 Paysans ; Jezebel 5411, Hunter, armée [3:39,11:4,7:50,20:5,46:4]. Besoin anticipé : six Paysans.
Le fixture conserve ces positions, ces troupes et le terrain de maps/Utopeia.fh2m. Objets, brouillard et attributs du héros contrôlés : ce n'est pas un rechargement de la sauvegarde originale. Le planificateur mesure collecte 400, livraison 2150 (2250 dans le contrôle séparé avec Pathfinding Basic). La v15 refuse au plafond 2000, malgré des capacités de chargement et réception satisfaites.

## Correctif borné

Livraison terrestre admise jusqu'à 4000 points après une collecte limitée à 1000 ou à la mobilité maximale du héros si inférieure ; total maximal 5000. Quatre jours chargés à la mobilité minimale du Paysan, cohérents avec le délai existant de quatre jours par étape. Aucun recrutement de héros spécifique.
Un visiteur ami à plus d'une journée de trajet ne renvoie plus immédiatement le porteur. Le héros continue son approche à terre ; la grâce existante d'un jour s'applique une fois proche. Danger, capacité, propriété, réservations de stocks/cargaisons, expiration et repli restent contrôlés. Un chargement impossible reste refusé, sans création ni suppression de troupes.
POPULATION_SUPPLY_PLAN expose pickup_cost, delivery_cost, delivery_budget.
Ouverture J1, navigation après J1, constructions, coûts par défaut et INI inchangés.

## Validation acquise

Compilation native stricte réussie. 154 exécutions natives : 14 v16, 48 régressions v15, 64 population/équipages v13, 28 économiques/navigation v9. Diagnostics 0 et 2 : résultats identiques. Conservation par type vérifiée sur les transferts structurés.
Collecte de six Paysans à J143, livraison à J145, puis 10/10 conscriptions J144–147, contre 7/10 sans livraison. Un seul plan. Le scénario passe aussi par KingdomTurn, visites et mouvements natifs ; seuls le dessin et les événements de bureau sont remplacés. Ce tour complet a révélé puis vérifié le cas du visiteur distant.
Refus testés : ville menacée, héros militaire, chargement impossible. Eau, capture, pénurie, réservation, délai, garnisons pleines et ouverture maritime J1 restent couverts par les régressions.
Résultats : knight-ai-v16-validation.json. Cela prouve une livraison utile dans une reproduction contrôlée, sans garantir toutes les livraisons des parties originales.

## Prochain test et reprise

Utopeia J1–168/175 : vérifier collecte, arrivée, conscription et absence de doublons. AoW : Blackfang/Baywatch sur la durée, et ouverture maritime J1 inchangée.
État : production et tests locaux terminés. Compilation et vérification du paquet Windows encore à terminer avant livraison. Ne pas les annoncer réussies à ce point.
