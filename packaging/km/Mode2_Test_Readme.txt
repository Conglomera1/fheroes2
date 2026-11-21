KNIGHT MOD - ADJUSTABLE CONSCRIPTION TEST - Windows x86

Close the game. Extract this ZIP into a working 32-bit fheroes2 installation,
keeping folders intact. Start fheroes2KM.exe using your existing Heroes II data
and matching SDL2 runtime.

Enable the new mode in knight_mod.ini, in the fheroes2 configuration folder.
The file is generated when you start or load a game. Close the game and set:

    conscription.mode = 2
    conscription.gold_per_peasant = 50
    conscription.gold_increment = 1.0
    conscription.gold_increment_multiplier = 1.0
    conscription.gold_increment_step = 1
    conscription.uniform_batch_price = 0
    training_grounds.gold = 1750
    training_grounds.wood = 10
    training_grounds.ore = 5
    farm_reconversion.gold = 1250

Existing explicit values are kept. knight_mod.ini.default is a reference.
The ordinary fixed schedule remains mode 1. Conscription is disabled in mode 0.

Open a Knight town's Training Grounds and select Change. Set an order of 4.
The full order quotes 500 gold in training fees. Confirm with Okay, then
end the turn. The order persists. A new Training Grounds starts with 0.
If stock or gold is short, the town processes the affordable part of the
order. Recruits taken from the Hut cost another 20 gold each.
Try setting uniform_batch_price to 1 and restarting. The full order then
quotes 800 gold. If only 3 garrison Peasants are available, it charges
450 gold and leaves the order at 4.

The game writes fheroes2.log beside knight_mod.ini. Leave
"knight diagnostics = 1" in fheroes2.cfg for daily results and AI decisions
(0 disables logging). Use a fresh log per test if possible. Observer access
can be enabled with "knight observer = on" in the same config.

Use separate save slots. A save created with this test stores each town's
order and requires the test build or a later compatible version.
