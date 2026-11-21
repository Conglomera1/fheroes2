================================================================
                           KNIGHT MOD                           
                          Version 1.0b                          
                  Publiée le: 21 novembre 2026
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

Cette bêta reprend l’alpha non publiée et y ajoute la prise en charge
par l’IA, les traductions et diverses corrections.

Que fait le mod ?
-----------------

- Taxpayer : Chaque Paysan stationné dans une ville ou un château
  Chevalier disposant d’une Ferme rapporte un revenu quotidien (2 pièces
  d’or par défaut).

- Conscription de Paysans : Moyennant un coût en or et en ressources,
  une ferme peut être convertie en terrain d’entraînement et, au besoin,
  reconvertie en ferme. Chaque jour, les terrains d’entraînement forment
  automatiquement des Paysans en Archers, Piquiers et Spadassins. Par
  défaut, une semaine complète permet de convertir 17 Paysans en 8
  Archers, 5 Piquiers et 4 Spadassins. La formation consomme des Paysans
  et entraîne des frais de formation (100 pièces d’or par Paysan par défaut).
  Les soldats ainsi formés rejoignent les demeures de recrutement de la ville
  et doivent encore être achetés à leur prix habituel. Tant que la Ferme est
  convertie en terrain d’entraînement, elle ne fournit plus son bonus de 
  croissance et les Paysans de cette ville ne rapportent plus de taxes.

- Personnalisation : L’objectif principal de ce mod est d’expérimenter
  afin de trouver le bon équilibre entre les deux mécaniques. Un fichier
  de configuration INI modifiable entre les parties permet d’essayer
  différentes règles de taxation et de conscription. Voir la section
  CONFIGURATION.

- Prise en charge par l’IA : Les joueurs contrôlés par l’ordinateur
  peuvent utiliser les deux mécaniques. Des adaptations spécifiques leur
  permettent de gérer les Paysans, le développement des villes, la
  taxation et la conscription.

Que ne fait-il pas ?
--------------------

- Il ne change ni le prix ni les statistiques de combat des Paysans.
- Il ne garantit ni des décisions optimales de l’IA ni un équilibrage
  définitif de la faction.
- Le café (pas encore).

Il s’agit d’une version bêta. Les retours sur l’équilibrage, la vitesse
de formation, les prérequis, les décisions de l’IA et le jeu Chevalier
en partie longue sont les bienvenus.

Consulter Guide_de_Test_et_Equilibrage.pdf pour une liste complète des
points à tester et un guide pratique permettant d’affiner l’équilibrage
via l’INI.

----------------------------------------------------------------
                          INSTALLATION                          
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

Comment installer la version Windows ?
--------------------------------------

1. Fermer fheroes2 avant d’installer ou de mettre à jour le mod.
2. Choisir l’archive correspondant à l’installation de fheroes2 : x86
   pour la version 32 bits, ou x64 pour la version 64 bits.
3. Extraire son contenu dans le dossier d’installation de fheroes2, en
   conservant la structure des dossiers. Si Windows propose de fusionner
   les dossiers, accepter. En cas de mise à jour du Knight Mod,
   autoriser le remplacement des fichiers existants. Si le dossier
   d’installation est protégé, accorder l’autorisation administrateur
   demandée par Windows.
4. Lancer fheroes2KM.exe. Créer un raccourci sur le Bureau permet de
   faciliter l’accès au mod.

Le mod utilise des noms de fichiers distincts :

- fheroes2KM.exe
- files/data/resurrectionKM.h2d

Ses fichiers de traduction portent le préfixe km_ dans le dossier
files/lang.

L’exécutable original de fheroes2, ses fichiers de ressources et ses
traductions ne sont pas modifiés.

Besoin d’une version pour une autre plateforme ? Ne pas hésiter à me le
demander !

Comment désinstaller le mod ?
-----------------------------

Désinstallation automatique

1. Lancer Uninstall_KM.bat depuis le dossier de fheroes2 où est installé
   Knight Mod et suivre les instructions. Si Windows demande une
   autorisation administrateur, l’accorder pour poursuivre.
2. Après la suppression des fichiers du mod, le désinstalleur propose de
   supprimer knight_mod.ini, s’il le trouve. Ce fichier contient les
   réglages personnels du Knight Mod. Choisir N pour les conserver en
   vue d’une prochaine utilisation, ou Y pour les supprimer.
3. Uninstall_KM.bat se supprimera lui-même une fois la désinstallation
   terminée.

Désinstallation manuelle

Fermer le jeu, puis supprimer les fichiers suivants du dossier
d’installation :

- fheroes2KM.exe
- files/data/resurrectionKM.h2d
- files/data/training_grounds.png
- files/data/build_training_grounds.png
- Les fichiers du dossier files/lang dont le nom commence par km_ et se
  termine par .mo.
- Uninstall_KM.bat

Pour supprimer également les réglages personnels du mod, supprimer :

  %APPDATA%\fheroes2\knight_mod.ini

Conserver ce fichier permet de retrouver ses réglages lors d’une
prochaine utilisation. S’il est supprimé, le mod le recréera avec les
valeurs par défaut au prochain démarrage ou chargement d’une partie.
Consulter « Où se trouvent les réglages ? » si la configuration utilise
un autre emplacement.

Windows peut demander une autorisation administrateur pour supprimer des
fichiers dans un dossier protégé, comme Program Files.

L’exécutable original de fheroes2, resurrection.h2d, les traductions et
les sauvegardes restent en place.

Peut-on utiliser des sauvegardes existantes ?
---------------------------------------------

Cette bêta peut charger les sauvegardes compatibles de fheroes2. Les
sauvegardes créées avec cette bêta nécessitent cette version ou une
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

Où se trouvent les réglages ?
-----------------------------

Lancer le mod et commencer ou charger une partie. Le fichier
knight_mod.ini est créé dans le dossier de configuration utilisateur de
fheroes2. Sous Windows, il se trouve normalement ici :

  %APPDATA%\fheroes2\knight_mod.ini

Pour ouvrir le fichier, appuyer sur Windows + R, coller le chemin
ci-dessus et appuyer sur Entrée.

Si une variable d’environnement HOME est définie dans Windows, le mod
utilise cet emplacement à la place :

  %HOME%\.fheroes2\knight_mod.ini

Modifier le fichier knight_mod.ini situé dans ce dossier de
configuration pour changer les réglages utilisés par le jeu.

Quels réglages peut-on modifier ?
---------------------------------

Consulter Guide_de_Test_et_Equilibrage.pdf pour la liste complète et les
explications.

L’INI permet de régler le taux de taxation, le nombre maximal de
contribuables par Ferme, les frais de conscription, les limites
quotidiennes, la réussite des formations, la composition des recrues,
les coûts de conversion et les prérequis supplémentaires pour la
construction des terrains d’entraînement.

Un court explicatif (en anglais) de chaque paramètre se trouve déjà dans
knight_mod.ini.

Redémarrer le jeu après avoir modifié les réglages.

Pour rétablir les réglages par défaut, supprimer knight_mod.ini et
relancer le jeu. Un nouveau fichier contenant les valeurs par défaut
sera créé automatiquement au démarrage ou au chargement d’une partie.

----------------------------------------------------------------
                        SUITE DU PROJET                         
----------------------------------------------------------------

Quelles sont les prochaines étapes ?
------------------------------------

Cette bêta se concentre sur l’utilisation conjointe de Taxpayer et de
Conscription, y compris par les joueurs contrôlés par l’ordinateur.

La suite dépendra de l’intérêt suscité par le mod. Elle pourra inclure
de nouvelles améliorations de l’IA, des ajustements d’équilibrage, ainsi
que des améliorations des traductions et de l’interface, en fonction des
tests et des retours des joueurs.

Où envoyer les retours ?
------------------------

Pour signaler un problème ou donner son avis, répondre ou me contacter
(conglomera7769 sur Discord ou u/illustrious_cup8442 sur Reddit) sur la
plateforme où cette bêta a été trouvée, ou utiliser les issues GitHub du
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

https://github.com/Conglomera1/fheroes2/tree/knight-public-beta-20261118

Heroes of Might and Magic II et ses ressources originales appartiennent
à leurs propriétaires respectifs. Les données du jeu original ne sont
pas incluses.
