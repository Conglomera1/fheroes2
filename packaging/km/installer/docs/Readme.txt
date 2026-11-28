================================================================
                           KNIGHT MOD
                          Version 1.1b
                  Published: November 28, 2026
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

The first public beta combined both mechanics and added CPU support.
Version 1.1b introduces adjustable daily conscription in each town and
improves how CPU players supply and use their Training Grounds.

Two heroes from the original Heroes of Might and Magic are hiding in
Knight Mod. To find them, type their name on the build screen of a
castle of their faction. Look for a Barbarian who turned his back on
magic and a Knight rumored to have ties to a necromantic cult.

Recruit them and keep leveling them up. Both have a surprise in store.

What does it do?
----------------

- Taxpayer: In a Knight town or castle with a Farm, Peasants in the
  garrison or with a visiting hero earn 2 gold each per day by default.
  You can cap the number of taxpayers per Farm. A cap of 0 means
  unlimited.

- Peasant Conscription: You can turn a Farm into Training Grounds and
  convert it back when you need the Farm again. While the Training
  Grounds are active, the town loses the Farm’s Peasant growth bonus and
  its Peasants stop paying tax. Training uses Peasants from the garrison
  first, then from the Thatched Hut at their usual recruitment price
  on top of the training fee. The resulting Archers, Pikemen and
  Swordsmen are added to the town’s recruitment pools and still have
  to be bought at their normal prices.

- Computer players use both mechanics. In Adjustable in-game mode, they
  set their own orders and budget for training. The 1.1b changes also
  help them retain Peasants needed for training when recruiting other
  troops to reinforce a garrison.

How does daily conscription work?
---------------------------------

- Adjustable in-game: Open the Training Grounds in a town and choose
  Change to set how many Peasants to train each day. The window shows
  the expected troops and cost for the next day. The order stays in
  place until you change it. Set it to 0 to pause training in that town.
  Available Peasants and gold determine how much of an order can be
  filled. The charge is based on the number actually taken for training.

- Fixed quotas: Training runs automatically at a flat fee, with a limit
  of 2 Peasants on odd days of the week and 3 on even days by default.
  Under the standard troop mix and with enough Peasants and gold, a full
  week turns 17 Peasants into 8 Archers, 5 Pikemen and 4 Swordsmen.

- Disabled: Turns off conscription. Taxpayer can be configured
  independently.

In Adjustable in-game mode, the training fee per Peasant can rise as
the daily order moves through price tiers. With a base fee of 50 gold,
a 50-gold increase at each tier and one Peasant per tier, training
three Peasants from the garrison costs 300 gold (50 + 100 + 150).
The optional uniform price charges all three at the last tier, for
450 gold in this example. Each town starts again at the base fee
the next day.

What doesn't it do?
-------------------

- It does not change Peasant prices or combat statistics.
- It does not guarantee optimal CPU decisions or final faction balance.
- Still no coffee.

This is a Beta release. Feedback on balance, training speed, building
requirements, CPU decisions and long-term Knight gameplay is welcome.

Testing_and_Balance_Guide.pdf covers what to look for in a test game
and how to adjust the balance. See the conscription section above for
the new pricing options in 1.1b.

----------------------------------------------------------------
                        GETTING STARTED
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

How do I install and start Knight Mod on Windows?
-------------------------------------------------

With fheroes2_KM-1.1b_Windows_Setup.exe, select the folder containing fheroes2.exe.
For manual installation, follow Installation.txt in the main ZIP.

Open KnightModLauncher.exe from the installation folder or use the
shortcut created by the installer. The launcher lets you adjust the
settings before clicking Play. Run fheroes2KM.exe from the same folder
to start Knight Mod directly.

The two executables sit beside fheroes2.exe. Knight Mod keeps its
resources in Knight Mod\files and its Readme, guide and licence in
Knight Mod\docs. The launcher opens the documents in its current
language.

* Need a build for another platform? Ask me while I’m still motivated!

How do I uninstall?
-------------------

Close the game and the launcher. For a setup installation, choose
Uninstall Knight Mod from the Start menu. For a ZIP installation, run
Uninstall_KM.bat or follow the manual steps in Installation.txt. Both
uninstallers offer to remove the mod’s settings as well.

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

Using the launcher
------------------

Knight Mod Launcher is where you change the mod’s settings before
starting the game. The Settings tab covers taxation and the main
conscription options. Advanced contains training success, troop mix,
conversion costs and Training Grounds requirements. Hover over a setting
for an explanation and its default value.

The launcher defaults to Adjustable in-game with a base fee of 50 gold.
The Default button next to the base fee restores 100 gold in Fixed
quotas mode. The price example starts at three garrison Peasants.
Change the quantity to compare totals before setting an order in game.

Use Default next to a setting to restore just that value, or Restore
all defaults to reset the whole form. Save writes the settings. Play
saves them and opens the mod. Restart the game to apply changes made
while it is running.

Editing the INI manually
------------------------

The launcher saves to knight_mod.ini, in the fheroes2 user
configuration folder. On Windows, the normal path is:

  %APPDATA%\fheroes2\knight_mod.ini

If the HOME environment variable is set, the game uses:

  %HOME%\.fheroes2\knight_mod.ini

You can edit the file manually between sessions. If it is missing,
the game creates it when you first start or load a game. The file
contains short English comments explaining the options. The
conscription.mode setting uses 0 for Disabled, 1 for Fixed quotas and
2 for Adjustable in-game.

----------------------------------------------------------------
                            FUTURE
----------------------------------------------------------------

What's next?
------------

This beta is a chance to test tiered conscription in real games and
see how it changes the choice between farming and training troops.

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
https://github.com/Conglomera1/fheroes2

Heroes of Might and Magic II and its original assets belong to their
respective owners. Original game data is not included.
