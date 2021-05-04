# EE319k_SpaceInvaders

update log 

4/28/2021 4:11 AM

-generated rows of aliens & added a function AlienMove() to make aliens move from side to side.

-doesnt do anything else, can prob use this code later once we hav other stuff working

4/28 7:53 PM

-added player struct/spaceship that moves by taking input from slider

-value fluctuates a bit, will prob implement hardware averaging later to make it more stable

-will try to add buttons/missile firing mechanism later tonight

4/29 3:14 AM

-implemented button to shoot missile from spaceship (still need to make it edge triggered)

-once collisions are implemented (hopefully tmrw), basics of game engine will be done and we can polish up the game from there

4/29 7:52 PM

-collisions work, function isnt very general so will fix that later (won't work if alien layout is different)

-need to add sound, make button edge triggered, add timer stuff

5/1 4:43 AM

-added player lives, and aliens randomly shoot at player every 1 second (using timer1). can only get hit 6 times before dying (3 hearts w/ half hearts)

-added health for aliens & sprites reflecting how much damage done to an alien

-will add lose condition when hearts = 0 later

5/4 4:50 AM

-now displays score during & at end of game

-3 sounds: getting hit, destroying alien, and firing missile

-refactored some code, fixed some small bugs
