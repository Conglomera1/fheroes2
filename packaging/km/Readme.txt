================================================================
                       KNIGHT MOD
                    Alpha Player Release
================================================================

What is the Knight Mod?
-----------------------

The Knight Mod is an experimental fheroes2 modification that gives
the Knight faction a population-based economy and a way to mobilize
Peasants into infantry recruitment.

It combines the playable Taxpayer and Conscription mechanics.
The player interface supports both mechanics. CPU players do not
yet use Conscription as part of their economic strategy.

Designed and directed by Conglomera.
Coded and analysed with ChatGPT assistance.
Based on fheroes2 1.1.17.

How did it start?
-----------------

The idea was to give the Knight faction a population to manage:
keep Peasants working for tax income, or mobilize them to support
the town's military production.

After sharing and refining the idea with the community on Discord,
I decided to build it myself. Taxpayer and Conscription were
developed as separate mechanics and later brought together.

This Alpha is the first combined player release.

What does it do?
----------------

- By default, each eligible Peasant generates 2 gold per day.
- Peasants stationed in a qualifying Knight town or castle with a
  Farm count as taxpayers.
- Peasants in travelling hero armies do not generate taxes when they
  are outside a qualifying town.
- A Farm can be converted into Training Grounds when the configured
  requirements are fulfilled.
- Training Grounds automatically train a limited number of Peasants
  into Archers, Pikemen and Swordsmen.
- By default, training takes 2 Peasants on days 1, 3, 5 and 7 of the
  game week, and 3 on days 2, 4 and 6, per town. Training costs
  100 gold per Peasant from the garrison, or 120 from the hut.
  Training uses available Peasants and gold, without borrowing.
  Visiting heroes' Peasants are not conscripted.
- All conscripts complete their training by default. A configurable
  success ratio can introduce losses. Failed conscripts are lost,
  still paid for, and count towards the daily training limit.
- Trained troops enter the town's normal recruitment pools and must
  still be purchased at their regular prices.
- Each town keeps its own training cycle throughout the game,
  including after reconversion or town capture.
- Tax rates, training limits, conversion costs and building
  requirements can be customized through knight_mod.ini.
- The relevant information is integrated into the player interface.

What doesn't it do?
-------------------

- It does not change Peasant prices or combat statistics.
- It does not make CPU players use Conscription intelligently yet.
- It does not include translations in this Alpha release.
- It does not make the AI smarter overall. The existing Taxpayer AI
  support remains, while Conscription strategy is still player-only.

This is an Alpha release. Feedback on balance, training speed,
building requirements and long-term Knight gameplay is welcome.

----------------------------------------------------------------
                         INSTALLATION
----------------------------------------------------------------

What do I need?
---------------

A working fheroes2 1.1.17 installation with your Heroes II data.
The original game files are not included in this mod.

How do I install the Windows version?
-------------------------------------

1. Choose the package matching your existing fheroes2 installation:
   x86 for 32-bit, or x64 for 64-bit.
2. Extract its contents into your fheroes2 installation folder,
   keeping the folder structure.
3. If Windows asks whether folders should be merged, confirm it.
4. Launch fheroes2KM.exe.

The Alpha uses separate executable and data filenames:

  fheroes2KM.exe
  files/data/resurrectionKM.h2d

Your original fheroes2 executable and resurrection.h2d remain
untouched.

How do I uninstall?
-------------------

1. Run Uninstall_KM.bat from your fheroes2 installation folder
   and follow the instructions. The batch file deletes itself
   when the process is complete.

2. To uninstall manually, delete:

   - fheroes2KM.exe.
   - files/data/resurrectionKM.h2d.
   - files/data/training_grounds.png.
   - files/data/build_training_grounds.png.
   - knight_mod.ini from %APPDATA%\fheroes2.
   - Uninstall_KM.bat.

Your original fheroes2 executable, resurrection.h2d, translations
and saves remain in place.

Can I use existing saves?
-------------------------

This Alpha can load supported fheroes2 and earlier Conscription
prototype saves. Saves written by this Alpha require this version
or a compatible Knight Mod version. Keep separate copies if you
want to continue playing a game with another executable.

----------------------------------------------------------------
                            SETUP
----------------------------------------------------------------

Where are the settings?
-----------------------

Launch the mod and start or load a game. It creates knight_mod.ini
in your fheroes2 user configuration folder. On Windows, this is
normally:

  %APPDATA%\fheroes2\knight_mod.ini

To open the file, press Windows + R, paste the following path
and press Enter:

  %APPDATA%\fheroes2\knight_mod.ini

Which settings can I change?
----------------------------

The file controls taxation, conscription costs, daily limits,
training success, troop composition, conversion costs and additional
Training Grounds requirements.

The default Farm-to-Training-Grounds conversion costs 2500 gold,
10 wood and 10 ore. Returning to a Farm costs 1500 gold.
Both conversions use the town's construction action for the day.

The Farm, Archery Range, Blacksmith and Armory are always required.
Captain's Quarters are also required by default, but can be disabled.
Other building requirements can be enabled individually in the INI.

Building requirement switches use these values:

  0 = no
  1 = yes

Jousting Arena and Cathedral also support:

  2 = upgraded building required

With a value of 1, either the base or upgraded building qualifies.

The Mage Guild setting accepts 0 to disable the requirement, or
1 to 5 for the minimum required level. Higher levels also qualify.

The training success setting uses a decimal proportion:

  conscription.peasant_success_ratio = 1.0

1.0 means all conscripts succeed. At 0.9, nine out of every ten
conscripts succeed and one is lost, although all ten are paid for.
This is deterministic. Fractional progress carries over between days
for each town, including through saves, captures and reconversions.
Only successful training advances the Archer/Pikeman/Swordsman cycle.

The complete list of available settings and their default values
is written to knight_mod.ini automatically.

Restart fheroes2 after changing the settings.

To restore the mod's default settings, delete knight_mod.ini
and launch the mod again. A new file containing the default values
will be created automatically.

----------------------------------------------------------------
                            FUTURE
----------------------------------------------------------------

What's next?
------------

This Alpha focuses on the player experience and balance of Taxpayer
and Conscription.

Future work may include CPU support for Conscription, additional
balance adjustments, translations and further interface improvements.

Where can I share feedback?
---------------------------

Reply where you found the Alpha, or use the project's issues:
https://github.com/Conglomera1/fheroes2/issues

Mention the map, difficulty, settings and game day. A save from
before a problem is especially helpful.

----------------------------------------------------------------
                            THANKS
----------------------------------------------------------------

Thanks to the fheroes2 team for the engine; Winimaru and Picso for
the encouragement, support and ideas; The YellowDart for helping
reshape the concept and inspiring me to build it myself; and
YhyJasne for the annoying but often thoughtful questions.

----------------------------------------------------------------
                         LEGAL STUFF
----------------------------------------------------------------

The Knight Mod is an unofficial community modification for fheroes2.

fheroes2 and this modification are distributed under the GNU GPL,
version 2 or later. See LICENCE for the full terms. Source:
https://github.com/ihhub/fheroes2
https://github.com/Conglomera1/fheroes2/tree/knight-mod-alpha-player

Heroes of Might and Magic II and its original assets belong to
their respective owners. Original game data is not included.
