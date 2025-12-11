Ok here's what we want to accomplish. 

I want to create a dps calucaltor, fight simulatofor osrs. 

We already have a rudimentary player, monster and item class. 


We also implemented the abiltity to fetch currently equipped items from the wikisunc plugin , this is implemented in the main.cpp but should be moved to the player class. 

When we fetch the gear, we should ge the bonuses and added them to the player stats. the [l;ayer class only fetches the ids of the equipment, but these can be matches using amehtod in the player class. the items-complete is a json of items, indexed by id.

The player should have a gear attrivute. 

Then - create a 'Battle" class. That should simulate the fight (we have the atck roll , damage etc formulas implemented in the main cpp, these can be moved here)' 


Ultimately we want to create a simulater where we get: the players current equipped gear (fetched from the runelite wikisync plugin via the websocket function we impleted), a target monster (example 'Khazard warlordd') and a simulation of times to kill. 