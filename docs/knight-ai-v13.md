# Knight AI v13 : anticipation du ravitaillement et équipages mixtes

Les coûts restent inchangés : TG 2 500 or, 10 bois, 10 minerai ; retour Farm
1 500 or. Les valeurs du fichier INI continuent à s'appliquer. Aucun changement
aux comparaisons Farm/TG, aux engagements ni au seuil de récupération financière.

## Ravitaillement

Une TG sûre calcule son besoin initial en garnison pour couvrir quatorze jours
selon les limites de conscription configurées, les stocks de Hut et les dates de
croissance normale Hut/Well. Les paysans déjà en garnison et les livraisons
valides déjà affectées sont déduits de ce besoin. Les chargements encore à
collecter réservent aussi le stock de leur Farm source.

Un convoyeur, éclaireur ou chasseur existant peut recevoir un ordre de collecte
suivi d'une livraison depuis une Farm arrière sûre. La collecte est limitée à
1 000 points de mouvement terrestres ; la livraison à 2 000 ; l'ensemble à 3 000
pour un convoyeur, 1 000 pour les autres rôles. Les combattants, patrouilles,
héros endormis, navigateurs affectés et héros en ville menacée sont exclus. Aucun
héros n'est acheté pour la logistique. Les ordres militaires critiques gardent
leur priorité dans le planificateur.

Les réservations sont annulées si la destination ou la source ne convient plus,
si elle est occupée, si le héros disparaît ou si le délai de quatre jours est
dépassé. Les paysans réellement chargés conservent le dépôt de repli sûr de v12.
À la collecte, le besoin est recalculé ; une livraison terminée n'est plus
comptée comme une cargaison en route. Aucun transfert à distance n'est réalisé.

Cette politique n'est pas un réseau de transport général. Un manque peut
subsister sans héros admissible ou sans itinéraire court sûr. Les cargaisons ne
sont pas considérées comme déjà présentes avant l'arrivée : une livraison trop
tardive peut encore laisser un jour sans conscription.

## Équipages de la première exploration maritime

Uniquement durant l'ouverture maritime déjà définie par v12, le navigateur
conserve cinq stacks d'un individu, trois N1 et deux N2 de sa faction :

| Faction | 1 | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- |
| Knight | Archer | Peasant | Peasant | Peasant | Archer |
| Barbarian | Orc | Goblin | Goblin | Goblin | Orc |
| Sorceress | Sprite | Dwarf | Sprite | Dwarf | Sprite |
| Warlock | Centaur | Gargoyle | Centaur | Gargoyle | Centaur |
| Wizard | Halfling | Boar | Halfling | Boar | Halfling |
| Necromancer | Skeleton | Zombie | Skeleton | Zombie | Skeleton |

Une amélioration N2 existante peut remplacer sa version de base. Les unités
restantes vont dans la garnison du port. Si les créatures ou les places à terre
manquent, la préparation spéciale est refusée sans transfert partiel et sans
achat automatique ; la politique ordinaire de départ s'applique. Aucun équipage
de cinq paysans isolés n'est créé par cette règle.

L'ordre des stacks est préservé par la préparation native du combat durant le
premier voyage et l'exception cesse au débarquement. Elle ne garantit pas la
fuite. Le recrutement préfère toujours une Sorceress, puis la faction du port,
parmi les deux candidats réellement disponibles. Le premier héros reste
réservé à la terre lorsque l'ouverture avec un deuxième héros est réalisable.

## Télémétrie

`ai_revision: 13`. Nouveaux événements : `POPULATION_SUPPLY_PLAN`,
`POPULATION_SUPPLY_REVIEW`, `NAVAL_OPENING_CREW_WAIT`. Les annulations de route
exposent la collecte et sa quantité. Les transferts maritimes exposent les deux
types de créatures et les armées avant/après. Les pouvoirs d'observation humaine
ne modifient pas la connaissance de la carte par l'IA.

## Validation native

Compilation complète réussie, avertissements traités comme erreurs.
**306 processus de test réussis**, diagnostics désactivés puis activés :

| Groupe | Exécutions |
| --- | ---: |
| Nouveaux cas v13 | 64 |
| Régressions v12 | 38 |
| Régressions v11, adaptées aux équipages mixtes | 38 |
| Régressions v10, adaptées aux équipages mixtes | 34 |
| Régressions v9 | 28 |
| Régressions v8 | 30 |
| Régressions v7 | 34 |
| Adaptateur Farm/TG | 20 |
| Constructions | 20 |
| Total | 306 |

Les nouveaux cas comprennent la collecte et la livraison physiques, la sélection
réelle de la cible prioritaire, le renforcement natif lors des visites, deux
héros face au même besoin, plusieurs TG face au même stock, une cargaison déjà
chargée, les refus pour danger/occupation/mer/stock et les annulations pour
capture/délai/urgence. Les limites 5/4 de l'INI sont testées séparément des
limites 2/3 par défaut. Les six formations, un N2 amélioré, les effectifs
insuffisants et la saturation de garnison sont vérifiés sans perte d'unités.

Les anciennes régressions couvrent notamment l'achat du deuxième héros,
Statue et bateau, suivi d'un embarquement natif à J1 avec trois Sprites et deux
Dwarfs. Les contrôles Python vérifient la conservation des armées dans chaque
transfert journalisé et l'identité des résultats avec/sans diagnostics.

Il s'agit d'états de moteur contrôlés sous Linux, pas d'une nouvelle partie
complète AoW/Utopeia ni d'un essai manuel sur Windows 7. La mémoire des missions
reste transitoire et se réinitialise au chargement.

## Prochains tests

- Utopeia depuis J1 : stocks des TG, départs et arrivées de convois, pénuries,
  doublons et durée des spécialisations ; conserver les coûts actuels.
- AoW depuis J1 : journal dès J7–14 pour l'ouverture, puis poursuite de la partie
  afin d'observer les objectifs en mer et les premiers débarquements.
