# Knight AI v15

Corrections issues de logs8.rar et Audit_v14_AoW_Utopeia.md.

## Ouverture J1

Accepter aussi OBJ_COAST comme case de réunion, pas seulement OBJ_NONE.
Prévoir avant achat un deuxième pas légal permettant de dégager l'entrée.
Après recrutement et partage réel : le héros terrestre paie ce pas de dégagement, puis le navigateur reçoit sa mission et peut embarquer. Les priorités de navigation après J1 et les coûts restent inchangés. Les motifs de refus précoces et NAVAL_OPENING_CLEARANCE rendent l'enchaînement observable.

## Destination momentanément occupée

Pour une livraison chargée vers une ville productive sûre, une occupation amie peut déclencher un placement sur une case adjacente vide, visible et accessible à terre en un tour. La destination et la cargaison sont conservées pendant le jour du premier blocage. Si le visiteur part, la livraison reprend ; s'il reste le lendemain, les replis antérieurs s'appliquent. Capture, danger, délai global et urgences militaires restent traités. Aucun héros n'est recruté pour cela.

## Cycle temporaire

La projection Farm→TG→Farm sur sept jours doit aussi laisser une trésorerie finale au moins égale à conserver la Farm, en plus du gain militaire supérieur à la marge existante de 10 %. La comparaison comprend les coûts réels INI, conscription, recrutement et revenu fiscal. Hilltop observée (27097 or projetés pour le cycle, contre 37326 pour Farm) ne satisfait plus ce critère. Cela n'impose pas un retour à Farm aux TG durables.

## Validation et reprise

Voir docs/knight-ai-v15-checkpoint.md pour l'état réellement atteint ; ne pas inférer un succès d'un test encore en préparation. Tests natifs de politique, conservation des effectifs et neutralité des diagnostics dans tests/run_knight_v15.py ; régressions économiques v9 et population v13.

Le fixture opening_full appelle KingdomTurn et la boucle normale des héros. Pour fonctionner sans images Heroes II, les wrappers de lien ne neutralisent que le dessin et les événements de bureau (pas de demande de quitter dans le test). Ils ne remplacent ni l'IA, ni le déplacement, ni les achats, ni le monde. Le fixture reste une carte synthétique à côte contrainte, pas une relecture intégrale d'AoW.
