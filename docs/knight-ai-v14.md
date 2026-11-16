# Knight AI v14 : ouverture J1 coordonnée et réception des convois

Cette version vise les parcours qui restaient incomplets en v13 : une collecte
utile sur une distance ordinaire d’Utopeia, la réception dans une TG pleine,
et le partage effectif des troupes dès J1 dans les ouvertures AoW/CoX.

## Ce qui change à J1

Pour un départ sûr avec un héros, une ville et un chantier naval déjà construit,
si un débouché maritime connu rend l’ouverture réalisable :

1. La Statue est financée et construite si nécessaire.
2. Le prix réel du deuxième héros et du bateau est conservé avant chaque
   recrutement facultatif. Les Archers Knight disponibles peuvent renforcer
   l’ouverture ; les Paysans recrutables peuvent compléter les contribuables
   d’une Farm déjà construite.
3. Le premier héros libère le château par un seul pas terrestre légal vers une
   case adjacente vide et visible. Le coût de mouvement est payé par le moteur.
4. Le navigateur est recruté parmi les deux candidats effectifs : Sorceress,
   puis classe du port, puis valeur native à priorité égale.
5. Les trois armées, héros terrestre, navigateur et garnison, fournissent le
   petit équipage. Tout le surplus est effectivement remis au héros terrestre
   adjacent avant sa poursuite du tour.
6. Le bateau reçoit sa mission selon la politique navale existante ; le
   navigateur peut rejoindre et embarquer durant le même J1.

La préparation prend cinq individus, trois N1 et deux N2. Elle préfère les
créatures de la faction du navigateur. À J1, les N2 manquants peuvent être pris
dans les autres armées disponibles : Gem peut ainsi recevoir deux Archers
Knight lorsque les Dwarfs sont absents. Les deux tireurs occupent les flancs ;
quand les trois N1 sont tireurs, le troisième est au centre. Les formations
Sorceress/Necromancer alternées restent les choix normaux avec leurs N2 natifs.
Aucune amélioration ni créature manquante n’est créée par cette règle.

Avec une Farm, les Paysans sont préférentiellement déposés pour la taxation.
Le héros terrestre les conserve si leur retrait fait refuser un combat visible,
accessible pendant le tour, que l’évaluation native accepte avec eux. Une armée
composée uniquement de Paysans les conserve également pour rester valide.
Cette exception militaire est protégée des dépôts ordinaires jusqu’à la fin de
J1 et expire à J2. Elle ne prédit pas les ennemis cachés.

Le budget de cette opération J1 peut laisser moins de 2 000 or, lorsque tous les
achats obligatoires sont couverts. La réserve ordinaire des tours suivants,
les engagements TG et les priorités de construction générales restent ceux de
v13. Une préparation impossible n’achète pas un bateau laissé sans cet équipage.
Les achats déjà utiles, notamment Statue et héros, ne sont pas annulés.

## Ravitaillement des TG

Les besoins continuent d’utiliser quatorze jours de conscription configurée,
les stocks réels de Hut, la garnison, les dates de croissance normale et les
livraisons affectées. Les cargaisons et les collectes promises réservent le
besoin ainsi que leur stock source ; un autre héros ne recharge pas le même
manque.

Les Farms sûres peuvent fournir le petit stock tampon même si leur rôle est
encore « primaire ». Les convoyeurs sont préférés aux éclaireurs, puis aux
chasseurs. Ces trois rôles partagent désormais les limites : collecte à
1 000 points de mouvement au plus, livraison à 2 000, total à 3 000. Le plafond
précédent de 1 000 au total pour les éclaireurs/chasseurs empêchait des livraisons
ordinaires d’environ deux jours. Un héros attendant dans un lieu sûr peut être
réaffecté. Les combattants, champions, patrouilles, trajets maritimes et urgences
militaires demeurent exclus ou prioritaires.

La capacité de chargement et de réception est testée sur des copies d’armées.
Une TG pleine peut recevoir les Paysans en échangeant un stack militaire entier
avec le convoyeur, notamment en utilisant la place libérée par le dépôt de la
cargaison. La mutation est atomique : aucun stack n’est supprimé pour libérer
une place, aucune créature n’est téléportée, aucun échange impossible n’est
appliqué partiellement. L’arrivée recalcule la situation réelle. Les destinations
occupées, capturées ou devenues dangereuses gardent les règles d’annulation et
de repli sûr ; une livraison refusée n’est pas marquée comme effectuée.

Cela augmente les possibilités de livraison, sans garantir une conscription
sans interruption : un héros peut rester trop loin, une urgence peut interrompre
le trajet, et les événements futurs inconnus ne sont pas anticipés comme des
certitudes.

## Journal

La révision enregistrée est **14**. Les événements d’ouverture sont
`NAVAL_OPENING_J1`, `NAVAL_OPENING_PARTY` et `NAVAL_OPENING_PEASANTS`. Les trois
armées avant/après permettent de rapprocher l’équipage, les renforts et les
contribuables. Les revues de ravitaillement détaillent les refus : disponibilité,
rôle militaire, source occupée ou vide/réservée, distance, accessibilité, mer et
capacité de chargement/réception. Les cibles menacées ou occupées sont également
signalées avant exclusion.

`TOWN_GROWTH` donne les stocks recrutables avant/après la croissance réellement
appliquée, avec le type de semaine et le drapeau de Peste. Les instantanés du
royaume indiquent aussi le type de semaine. Une Peste n’a donc plus à être
reconstituée à partir de divisions de stocks. Cette télémétrie ne donne pas à
l’IA d’informations cachées supplémentaires.

## Validation et limites

Les tests d’ouverture reprennent les inventaires et budgets observés dans
`AoW(1).log` et `CoX.log`, sur des terrains côtiers contrôlés de mêmes dimensions.
Ils appellent le parcours de production : construction, achats, déplacement
terrestre payé, recrutement, partage, choix de cible et embarquement natif.
Ils ne sont pas des relectures des cartes originales octet pour octet.

| État reproduit | Navigateur et équipage | Héros terrestre après partage | Farm | Or restant |
| --- | --- | --- | --- | ---: |
| AoW | Ector : A, P, P, P, A | Tyro : 70 Paysans, 6 Archers | Absente | 2 750 |
| CoX | Gem : A, S, S, S, A | Roland : 10 Archers, 17 Sprites | 61 Paysans | 1 310 |

Les deux navigateurs embarquent à J1 avec zéro mouvement restant et leurs
formations conservées. Le héros terrestre garde des mouvements disponibles.
Les stocks ramassés ensuite sur la carte ne sont pas supposés dans ces budgets.
Le cas de combat insuffisant conserve les Paysans et vérifie leur protection
pendant J1 ; le défaut de N2, le manque d’argent, l’absence de débouché connu,
le danger, le contrôle humain et un début à J2 sont traités séparément.

Le ravitaillement est testé avec un Hunter sur les coordonnées 5678 → 5841 d’une
carte de 144 cases de large : vraie route terrestre sur deux jours, cible pleine,
stock source réservé, deuxième héros non affecté au même besoin et arrivée
réelle. Un cas supplémentaire reprend les six Paysans, le stock de Hut et les
armées observées à Alamar/Elk’s Head à J192. La géométrie des itinéraires reste
contrôlée dans ces fixtures. La Peste est forcée dans un état de moteur distinct
pour vérifier le relevé de croissance.

Les régressions des versions précédentes sont exécutées avec diagnostics
éteints puis activés. Les résultats doivent être identiques ; les transferts
à deux armées et le partage à trois armées sont rapprochés pour conservation
des effectifs. Le nombre final et la provenance sont donnés dans le bilan de
livraison après validation.

Les coûts d’usine restent **TG : 2 500 or, 10 bois, 10 minerai ; retour Farm :
1 500 or**. L’INI n’est pas remplacé. Les critères de navigation après J1 ne sont
pas révisés. La mémoire des missions reste transitoire et se réinitialise au
chargement ; utiliser des sessions continues pour les comparaisons longues.
La correction du crash de la série d’autoplays ne fait pas partie de cette version.

## Essais conseillés

- AoW et CoX depuis J1 : conserver un premier journal dès la fin du tour pour
  vérifier le recrutement, les cinq stacks, les renforts terrestres, les
  contribuables éventuels et l’embarquement. La disponibilité réelle des héros
  proposés peut changer le résultat.
- Utopeia : test long pour vérifier les départs et arrivées de convois, les
  refus détaillés, les conscriptions manquées et les spécialités après capture.
