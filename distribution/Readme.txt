================================================================
                       KNIGHT MOD
                      Version 1.0b
            Release date : November 21, 2026
================================================================

What is the Knight Mod?
----------------------

The Knight Mod is an independent, experimental modification of fheroes2
that gives the Knight faction a population-based economy and a way to
mobilize Peasants into infantry recruitment. It tests ideas that could
be proposed to the fheroes2 Resurrection Team for possible inclusion in
future official releases.

It combines the playable Taxpayer mod (already released separately) with
Peasant Conscription and a dedicated, though still somewhat shaky,
computer player support system for both mechanics.

Designed and directed by Conglomera.
Coded and analysed with ChatGPT assistance.
Based on fheroes2 1.1.17.

How did it start?
-----------------

The idea was to give the Knight faction a population to manage: keep
Peasants working for tax income, or mobilize them to support the town's
military production.

After sharing and refining the idea with the community on Discord, I
decided to build it myself. Taxpayer and Conscription were developed as
separate mechanics and later brought together.

This Beta builds on the unreleased Alpha with the addition of CPU
support, translations and further fixes.

What does it do?
----------------

- Taxpayer: Each Peasant generates daily gold income (2 gold per day by
  default) when stationed in a Knight town or castle with a Farm.

- Peasant Conscription: Farms can be converted into Training Grounds and
  back. Each day, Training Grounds automatically train Peasants into
  Archers, Pikemen and Swordsmen. By default, a full weekly cycle
  converts 17 Peasants into 8 Archers, 5 Pikemen and 4 Swordsmen.
  Training consumes Peasants and gold (a training fee of 100 gold per
  Peasant by default), and the resulting troops are added to the town's
  recruitment pools, where they can be purchased normally. While a Farm
  is converted into Training Grounds, it no longer provides its Peasant
  growth bonus, and Peasants stationed in that town generate no tax income.

- Customization: Finding the right balance is the main goal of this
  experimental mod. Players can experiment with different taxation and
  conscription rules through a dedicated INI configuration file (see the
  SETUP section below).

- Computer Player Support: CPU players can use both mechanics, with
  dedicated AI logic to manage Peasants, town development, taxation and
  conscription.

What doesn't it do?
-------------------

- It does not change Peasant prices or combat statistics.
- It does not guarantee optimal CPU decisions or final faction balance.
- It doesn't serve coffee, yet.

This is a Beta release. Feedback on balance, training speed, building
requirements, CPU decisions and long-term Knight gameplay is welcome.

See Testing_and_Balance_Guide.pdf for a comprehensive testing checklist
and a practical guide to fine-tuning the balance via the INI.

----------------------------------------------------------------
                         INSTALLATION
----------------------------------------------------------------

What do I need?
---------------

A working fheroes2 1.1.17 or later installation with your Heroes II
data. The original game files are not included in this mod.

Knight Mod is based on fheroes2 1.1.17 and runs through its own
executable. It can be installed alongside a later fheroes2 version, but
it will still use its 1.1.17-based engine and will not benefit from
improvements or features introduced in those later versions.

The Windows package uses the runtime libraries from your existing
fheroes2 installation. Its architecture must match that installation.

How do I install the Windows version?
-------------------------------------

1. Close fheroes2 before installing or updating the mod.
2. Choose the package matching your existing fheroes2 installation: x86
   for 32-bit, or x64 for 64-bit.
3. Extract its contents into your fheroes2 installation folder, keeping
   the folder structure. If Windows asks whether folders should be
   merged, confirm it. If you're overwriting an existing Knight Mod
   installation, allow existing files to be replaced. If the
   installation folder is protected, approve the Windows administrator
   permission prompt.
4. Launch fheroes2KM.exe. A desktop shortcut can be created for easier
   access.

The mod uses separate executable and data filenames:

- fheroes2KM.exe
- files/data/resurrectionKM.h2d

Its language catalogs use the km_ prefix in files/lang.

Your original fheroes2 executable, resource and language files remain
untouched.

* Need a build for another platform? Feel free to ask!

How do I uninstall?
-------------------

Automatic removal

1. Run Uninstall_KM.bat from your fheroes2 installation folder and
   follow the instructions. If Windows requests administrator
   permission, approve the prompt to continue.
2. After removing the mod files, the uninstaller offers to delete
   knight_mod.ini if it finds the file. This contains your personal
   Knight Mod settings. Choose N to keep them for future use, or Y to
   delete them.
3. The batch file deletes itself when the process is complete.

Manual removal

Close the game, then delete the following files from your fheroes2
installation folder:

- fheroes2KM.exe
- files/data/resurrectionKM.h2d
- files/data/training_grounds.png
- files/data/build_training_grounds.png
- The files in files/lang whose names begin with km_ and end with .mo.
- Uninstall_KM.bat

If you also want to remove your personal Knight Mod settings, delete:

  %APPDATA%\fheroes2\knight_mod.ini

Keeping this file allows you to reuse your settings later. If it is
deleted, the mod will recreate it with default settings the next time
you start or load a game. See "Where are the settings?" below if your
configuration uses a different location.

Windows may request administrator permission to remove files from a
protected installation folder such as Program Files.

Your original fheroes2 executable, resurrection.h2d, translations and
saves remain in place.

Can I use existing saves?
-------------------------

This Beta can load supported fheroes2 saves. Saves written by this Beta
require this version or a compatible Knight Mod version.

Knight Mod saves cannot be loaded by the original fheroes2 executable.

The mod uses the same default save folder as fheroes2 and will therefore
overwrite existing autosaves and quicksaves. Save them under different
names before playing the mod so they remain available later in the
original version of fheroes2.

----------------------------------------------------------------
                            SETUP
----------------------------------------------------------------

Where are the settings?
-----------------------

Launch the mod and start or load a game. It creates knight_mod.ini in
your fheroes2 user configuration folder. On Windows, this is normally:

  %APPDATA%\fheroes2\knight_mod.ini

To open the file, press Windows + R, paste the following path and press
Enter:

  %APPDATA%\fheroes2\knight_mod.ini

If your Windows environment has a HOME variable defined, the mod uses
this location instead:

  %HOME%\.fheroes2\knight_mod.ini

Edit the active knight_mod.ini in your user configuration folder to
change the settings used by the game.

Which settings can I change?
----------------------------

- See Testing_and_Balance_Guide.pdf for a comprehensive list and
  details.

The INI file controls taxation rate, the maximum number of taxpayers per
Farm, conscription costs, daily limits, training success, troop
composition, conversion costs and additional Training Grounds
requirements.

The complete list of available settings and their default values is
written to knight_mod.ini automatically.

Restart the game after changing the settings.

To restore the mod's default settings, delete knight_mod.ini and launch
the game again. A new file containing the default values will be created
automatically when you start or load a game.

----------------------------------------------------------------
                            FUTURE
----------------------------------------------------------------

What's next?
------------

This Beta focuses on testing Taxpayer and Conscription together,
including their use by CPU players.

Future work will depend on interest in the mod and may include further
AI refinements, balance adjustments, translation improvements and
interface improvements, guided by testing and player feedback.

Where can I share feedback?
---------------------------

To report issues or share feedback, reply where you found the Beta,
contact me on Discord (conglomera7769) or Reddit (u/illustrious_cup8442),
or use the project's GitHub issues:
https://github.com/Conglomera1/fheroes2/issues

Ideas and suggestions for improving the mod are especially welcome!

----------------------------------------------------------------
                            THANKS
----------------------------------------------------------------

- The fheroes2 Resurrection team for bringing Heroes II back to life and
  for all the hard work and passion they continue to put into it;
- Winnymaru and Picso for the encouragement, support and ideas;
- TheYellowDart for helping reshape the concept, suggesting external
  configuration and inspiring me to build it myself;
- YhyJasne for the annoying but often thoughtful questions;
- The Poohs :) and the Picsopathes from Winnymaru's and Picso's Discord
  servers, mostly the same familiar faces... counting on you to put this
  mod through its paces!

----------------------------------------------------------------
                         LEGAL STUFF
----------------------------------------------------------------

The Knight Mod is an unofficial community modification for fheroes2.

fheroes2 and this modification are distributed under the GNU GPL,
version 2 or later. See LICENCE for the full terms. Source:

https://github.com/ihhub/fheroes2
https://github.com/Conglomera1/fheroes2/tree/knight-public-beta-20261118

Heroes of Might and Magic II and its original assets belong to their
respective owners. Original game data is not included.
