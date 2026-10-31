================================================================
                   PTP - PEASANT TAXPAYER MOD
                          Version 1.0b
================================================================

What is PTP?
------------

PTP stands for Peasant TaxPayer. It's a mod for fheroes2 that
gives Peasants a lasting role in your economy. Those useless
blokes can now help pay for the army!

Designed and directed by Conglomera.
Programmed and analysed by ChatGPT.
Based on fheroes2 1.1.17.

How did it start?
-----------------

The idea was to give the Knight faction a population to manage:
keep Peasants working for tax income, or mobilize them through
Conscription to increase infantry recruitment.

After sharing and refining the idea with the community, I decided
to try building it myself. The two mechanics are being developed
separately and will be brought together later.

This beta is the fully playable Taxpayer part of that project.

What does it do?
----------------

- By default, each Peasant stationed in a Knight town or castle
  with a Farm earns 2 gold per day. For example, 100 Peasants
  bring in 200 gold per day.
- Peasants in the garrison and in a hero's army stationed there
  count. Peasants travelling outside do not.
- Peasants recruited from external huts or wandering groups can
  become taxpayers once you bring them to a Knight town with a
  Farm.
- The same rules apply to human and CPU players, regardless of
  their starting faction. Owning a Knight town with a Farm is what
  matters.
- CPU players have been adapted to invest in Farms and Peasants,
  recruit from suitable huts and preserve their taxpayers, with
  exceptions for decisive defenses and true last stands.
- Taxpayer is integrated into the game's interface: the ability
  appears in the Peasant information window, projected tax income
  is included in the kingdom's total income, and Farm descriptions
  explain the benefit.
- The tax rate and Farm requirement are configurable (for beta
  testing).

What doesn't it do?
-------------------

- It doesn't change Peasant prices or combat stats.
- It doesn't include Conscription yet.
- It doesn't give the AI special map knowledge.

This is a beta: feedback on balance and AI behavior is welcome.

----------------------------------------------------------------
                          INSTALLATION
----------------------------------------------------------------

What do I need?
---------------

A working fheroes2 1.1.17 installation with your Heroes II data.
The original game files are not included in this mod.

How do I install the Windows version?
-------------------------------------

1. Choose the package matching your existing fheroes2
   installation: x86 for 32-bit, or x64 for 64-bit.
2. Extract its contents into your fheroes2 installation folder,
   keeping the folder structure, including files/lang and files/data.
3. Launch fheroes2TPx86.exe or fheroes2TPx64.exe.

Need a build for another platform? Feel free to ask!

The PTP executable, translations and resurrection_TP.h2d resource
use separate names, so
your original executable and language files stay in place. Both
executables still use the usual fheroes2 user settings and save
folders.

How do I uninstall?
-------------------

1. Copy uninstalltp.bat into your fheroes2 installation folder,
   run it and follow the instructions. It deletes itself when done.

2. To uninstall manually, delete:
   - fheroes2TPx86.exe and/or fheroes2TPx64.exe.
   - The files ending in _TP.mo inside files/lang.
   - files/data/resurrection_TP.h2d.
   - knight_mod.ini from %APPDATA%\fheroes2.
   - uninstalltp.bat.

Your original fheroes2 executable, translations, resources and
saves stay in place.

----------------------------------------------------------------
                             SETUP
----------------------------------------------------------------

Where are the settings?
-----------------------

Launch the mod and start or load a game. It creates knight_mod.ini
in your fheroes2 user configuration folder. On Windows, this is
normally: %APPDATA%\fheroes2\

To open the file, press Windows + R, paste the following path and
press Enter:

  %APPDATA%\fheroes2\knight_mod.ini

Which settings can I change?
----------------------------

  taxpayer.gold_per_peasant_per_day = 2

Daily gold per eligible Peasant. Use a non-negative whole number.
The default is 2. Set it to 0 to disable tax income.

  taxpayer.require_farms = 1

1 = Only Peasants stationed in Knight towns with Farms count
    (default).
0 = All Peasants you own count, including travelling armies
    and Peasants in other factions' towns.

Restart the game after changing these values.

----------------------------------------------------------------
                             FUTURE
----------------------------------------------------------------

What's next?
------------

First, feedback and balance testing for Taxpayer.

Then, Conscription: spending Peasants and gold to add Archers,
Pikemen and Swordsmen to the recruitment pools. Those troops would
still need to be purchased at their normal prices.

The aim is to make keeping a productive population and mobilizing
it both worthwhile choices.

Where can I share feedback?
---------------------------

Reply where you found the beta, or use the project's issues:
https://github.com/Conglomera1/fheroes2/issues

Mention the map, difficulty, tax settings and game day. A save
from before a problem is especially helpful.

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

PTP is an unofficial community mod for fheroes2.

fheroes2 and this modification are distributed under the GNU GPL,
version 2 or later. See LICENCE for the full terms. Source:
https://github.com/Conglomera1/fheroes2/tree/ptp-1.0b-windows

Heroes of Might and Magic II and its original assets belong to
their respective owners. Original game data is not included.
