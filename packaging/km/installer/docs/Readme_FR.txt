================================================================
                           KNIGHT MOD
                          Version 1.1b
                 Publication le 27 novembre 2026
================================================================

Qu’est-ce que Knight Mod ?
--------------------------

Knight Mod est une modification indépendante et expérimentale de
fheroes2 qui apporte à la faction Chevalier une économie fondée sur sa
population et un moyen de mobiliser les Paysans pour renforcer ses
réserves d’infanterie à recruter. Il permet de tester des idées qui
pourraient être proposées à l’équipe fheroes2 Resurrection pour une
éventuelle intégration dans de futures versions officielles.

Il réunit le mod Taxpayer, déjà jouable et distribué séparément, et la
mécanique de Conscription. L’IA a également été adaptée pour utiliser
ces deux mécaniques de façon globalement cohérente, même si son
comportement reste perfectible.

Conçu et dirigé par Conglomera.
Code et analyses réalisés avec l’aide de ChatGPT.
Basé sur fheroes2 1.1.17.

Comment le projet a-t-il commencé ?
-----------------------------------

L’idée était de donner à la faction Chevalier une population à gérer :
conserver les Paysans pour leurs revenus fiscaux, ou les mobiliser pour
soutenir la production militaire.

Après avoir partagé et affiné l’idée avec la communauté sur Discord,
j’ai décidé de la réaliser moi-même. Taxpayer et Conscription ont été
développés séparément, puis réunis.

La première beta publique réunissait les deux mécaniques et permettait
déjà à l’IA de les utiliser. La 1.1b ajoute la possibilité de régler la
conscription au jour le jour dans chaque ville et améliore la gestion
des terrains d’entraînement par l’IA.

Que fait le mod ?
-----------------

- Taxpayer : Dans une ville ou un château Chevalier doté d’une Ferme,
  chaque Paysan en garnison ou dans l’armée d’un héros en visite
  rapporte 2 pièces d’or par jour par défaut. Le nombre de contribuables
  peut être plafonné dans chaque Ferme. À 0, il est illimité.

- Conscription : Une Ferme peut être convertie en terrain d’entraînement,
  puis reconvertie en Ferme selon les besoins. Pendant la période
  d’entraînement, la ville perd le bonus de croissance de la Ferme et
  les Paysans qui y stationnent cessent de rapporter des taxes. Les
  Paysans sont prélevés en priorité dans la garnison. Ceux qui viennent
  de la Hutte coûtent aussi leur prix de recrutement habituel. Une fois
  formés, les Archers, Piquiers et Spadassins rejoignent les réserves de
  recrutement de la ville, où ils restent à acheter au prix normal.

- L’IA utilise les deux mécaniques. En mode Réglable en jeu, elle établit
  ses propres commandes et tient compte du coût de la formation. La
  1.1b améliore aussi la conservation des Paysans destinés à la
  conscription quand l’IA recrute d’autres troupes pour renforcer une
  garnison.

Comment fonctionne la conscription au quotidien ?
-------------------------------------------------

- Réglable en jeu : Ouvrir le terrain d’entraînement d’une ville pour
  fixer le nombre de Paysans à former chaque jour. La fenêtre indique
  les recrues et le coût prévus pour le lendemain. La commande reste
  active jusqu’à sa prochaine modification. Une quantité de 0 suspend
  la formation dans la ville. L’or et les Paysans disponibles
  déterminent la part de la commande qui peut être honorée. Seuls les
  Paysans effectivement prélevés sont facturés.

- Quotas fixes : La formation se fait automatiquement, à tarif fixe.
  Par défaut, la limite est de 2 Paysans les jours impairs de la
  semaine et de 3 les jours pairs. Avec la répartition standard et
  suffisamment de Paysans et d’or, une semaine complète permet de
  former 8 Archers, 5 Piquiers et 4 Spadassins à partir de 17 Paysans.

- Désactivée : La conscription est coupée. La taxation se règle
  séparément.

En mode Réglable en jeu, le coût par Paysan peut augmenter par paliers.
Avec un coût de base de 50 or, une hausse de 50 or et un Paysan par
palier, la formation de trois Paysans de garnison coûte 300 or
(50 + 100 + 150). L’option de tarif uniforme applique à tous le tarif
du dernier palier atteint, soit 450 or dans cet exemple. Chaque ville
repart du tarif de base le lendemain.

Que ne fait-il pas ?
--------------------

- Il ne change ni le prix ni les statistiques de combat des Paysans.
- Il ne garantit ni des décisions optimales de l’IA ni un équilibrage
  définitif de la faction.
- Le café (toujours pas).

Il s’agit d’une version beta. Les retours sur l’équilibrage, la vitesse
de formation, les prérequis, les décisions de l’IA et le jeu Chevalier
en partie longue sont les bienvenus.

Le Guide_de_Test_et_Equilibrage.pdf présente les points à observer en
partie et les leviers d’équilibrage. Le nouveau barème de conscription
de la 1.1b est expliqué plus haut.

----------------------------------------------------------------
                           DÉMARRAGE
----------------------------------------------------------------

Que faut-il pour jouer ?
------------------------

Une installation fonctionnelle de fheroes2 1.1.17 ou version ultérieure,
avec les données de Heroes II. Les fichiers du jeu original ne sont pas
inclus.

Knight Mod est basé sur la version 1.1.17 de fheroes2 et utilise son
propre fichier exécutable. Il peut être installé aux côtés d’une version
plus récente de fheroes2, mais reste fondé sur le moteur de la version
1.1.17. Il ne bénéficie donc pas des améliorations et ajouts des
versions ultérieures.

Le mod utilise les bibliothèques déjà présentes dans l’installation
Windows de fheroes2. Son architecture doit correspondre à celle de cette
installation.

Comment installer et lancer Knight Mod sous Windows ?
-----------------------------------------------------

Avec Knight-Mod-Setup.exe, sélectionner le dossier contenant
fheroes2.exe. Pour une installation manuelle, suivre les consignes
d’installation_fr.txt, à la racine du ZIP principal.

Ouvrir KnightModLauncher.exe depuis le dossier d’installation ou le
raccourci créé par l’installeur. Le lanceur permet de modifier les
réglages avant de cliquer sur Jouer. Pour lancer Knight Mod
directement, ouvrir fheroes2KM.exe depuis le même dossier.

Les deux exécutables se trouvent à côté de fheroes2.exe. Les ressources
du mod sont dans Knight Mod\files, et le Readme, le guide et la licence
dans Knight Mod\docs. Le lanceur ouvre les documents dans sa langue
d’affichage.

Besoin d’une version pour une autre plateforme ? Ne pas hésiter à me
le demander tant que je suis motivé !

Comment désinstaller le mod ?
-----------------------------

Fermer le jeu et le lanceur. Pour une installation par EXE, choisir
Désinstaller Knight Mod dans le menu Démarrer. Pour une installation
par ZIP, exécuter Uninstall_KM.bat ou suivre les étapes manuelles
d’installation_fr.txt. Les deux désinstalleurs proposent aussi de
supprimer les réglages du mod.

Peut-on utiliser des sauvegardes existantes ?
---------------------------------------------

Cette beta peut charger les sauvegardes compatibles de fheroes2. Les
sauvegardes créées avec cette beta nécessitent cette version ou une
version compatible du Knight Mod.

Les sauvegardes du Knight Mod ne peuvent pas être chargées avec
l’exécutable original de fheroes2.

Le mod utilise le même dossier de sauvegarde par défaut que fheroes2 et
écrasera donc les sauvegardes automatiques et rapides existantes.
Enregistrer d’abord AUTOSAVE et QUICKSAVE sous d’autres noms avant de
jouer au mod, pour pouvoir les retrouver ultérieurement dans la version
originale de fheroes2.

----------------------------------------------------------------
                         CONFIGURATION
----------------------------------------------------------------

Utiliser le lanceur
-------------------

Le lanceur permet de régler Knight Mod avant d’ouvrir le jeu. L’onglet
Réglages réunit la taxation et les principales options de conscription.
L’onglet Avancé donne accès au taux de réussite de la formation, à la
répartition des recrues, aux coûts de conversion et aux prérequis des
terrains d’entraînement. Survoler un réglage affiche son rôle et sa
valeur par défaut.

Le lanceur propose par défaut le mode Réglable en jeu, avec un coût de
base de 50 or. En mode Quotas fixes, le bouton Défaut de ce champ
rétablit le tarif de 100 or. L’exemple de coût part de trois Paysans
de garnison. Modifier leur nombre permet de comparer les tarifs avant
de fixer une commande en jeu.

Le bouton Défaut rétablit la valeur du réglage correspondant. Tout
remettre par défaut réinitialise le formulaire. Enregistrer écrit les
réglages, et Jouer les enregistre puis ouvre le mod. Si le jeu est déjà
ouvert, le redémarrer pour appliquer les changements.

Modifier le fichier INI manuellement
------------------------------------

Le lanceur écrit ses réglages dans knight_mod.ini, dans le dossier de
configuration utilisateur de fheroes2. Sous Windows, le chemin
habituel est :

  %APPDATA%\fheroes2\knight_mod.ini

Si la variable d’environnement HOME est définie, le jeu utilise :

  %HOME%\.fheroes2\knight_mod.ini

Le fichier peut aussi être modifié manuellement entre deux sessions. En
son absence, le jeu le crée au démarrage ou au chargement d’une partie.
Les commentaires en anglais décrivent les paramètres. Pour
conscription.mode, 0 désactive la conscription, 1 correspond à Quotas
fixes et 2 à Réglable en jeu.

----------------------------------------------------------------
                        SUITE DU PROJET
----------------------------------------------------------------

Quelles sont les prochaines étapes ?
------------------------------------

Cette beta permet d’essayer le barème par paliers en partie et de voir
comment il change l’intérêt de conserver une Ferme ou de mobiliser des
Paysans.

La suite dépendra de l’intérêt suscité par le mod. Elle pourra inclure
de nouvelles améliorations de l’IA, des ajustements d’équilibrage, ainsi
que des améliorations des traductions et de l’interface, en fonction des
tests et des retours des joueurs.

Où envoyer les retours ?
------------------------

Pour signaler un problème ou donner son avis, répondre ou me contacter
(conglomera7769 sur Discord ou u/illustrious_cup8442 sur Reddit) sur la
plateforme où cette beta a été trouvée, ou utiliser les issues GitHub du
projet :

https://github.com/Conglomera1/fheroes2/issues

Les idées et suggestions pour améliorer le mod sont particulièrement
bienvenues !

----------------------------------------------------------------
                         REMERCIEMENTS
----------------------------------------------------------------

- À l’équipe fheroes2 Resurrection pour avoir ressuscité Heroes II et
  continuer à le faire vivre avec tant de passion et de soin ;
- À Winnymaru et Picso pour les encouragements, le soutien et les idées ;
- À TheYellowDart pour avoir contribué à remanier le concept, suggéré la
  configuration externe et donné l’envie de le réaliser moi-même ;
- À YhyJasne pour les questions agaçantes, mais souvent pertinentes ;
- Aux Oursons et aux Picsopathes des serveurs Discord de Winnymaru et
  Picso, en grande partie les mêmes têtes… je compte sur vous pour
  mettre ce mod à l’épreuve !

----------------------------------------------------------------
                        MENTIONS LÉGALES
----------------------------------------------------------------

Knight Mod est une modification communautaire non officielle de
fheroes2.

fheroes2 et cette modification sont distribués sous licence GNU GPL,
version 2 ou ultérieure. Consulter LICENCE pour le texte complet.

Code source :

https://github.com/ihhub/fheroes2

https://github.com/Conglomera1/fheroes2

Heroes of Might and Magic II et ses ressources originales appartiennent
à leurs propriétaires respectifs. Les données du jeu original ne sont
pas incluses.
