# EndlessPacMan

My first crack at a console-based game in C++

### How to build/run:
I've only built and run this in Visual Studio so far, so I can't give much of a guide otherwise. It's recommended to run in release mode though as performance takes a massive
hit when run in debug mode, plus the score/level tracker displays some weird characters in debug mode (probably because it's using a larger buffer than it really needs).

### Enemy AI:
I've implemented the A* pathfinding algorithm for the enemy path-finding. Originally it worked off a kind of:

```
Is player above me?
	Go up
Is player below me?
	Go down
Is player to my right?
	Go right
Is player to my left?
	Go left
```

Which worked for the most part, until I started adding more complex levels with more walls, at which point enemies routinely became stuck and the game became far too easy.

The enemies re-evaluate their path on each iteration of the main game loop, whilst this is quite taxing on performance, it means the enemies always have the best route towards
the player at any one time.

### Difficulty:
There are 5 difficulty levels to the game:

* `EASY` - This is, as the name suggests, very easy. The enemies are extremely slow and can be sidestepped without much issue
* `MEDIUM` - A bit harder than easy, the enemies move a bit quicker but are still very slow in comparison to the player
* `HARD` - More of a challenge, but not too difficult. The enemies can corner the player but are still not fast enough to make the game extermely hard
* `VERY_HARD` - Enemies on this difficulty are fast, meaning the player will have to judge their route when picking up coins as to not be cornered by enemies
* `NIGHTMARE` - This level is essentially impossible. The enemies move as fast as the player does, and they have a pathfinding algorithm on their side, you don't.

As you have probably guessed, the difficulty affects purely the enemy speed at this point. This is done by adding an artificial 'delay' to the enemy movement. To do this
the enemies only move every X game loop iterations. On `NIGHTMARE` difficulty, they move on each game loop. Which makes them incredibly fast, and depending on where they spawn
they sometimes get to the player before they can even move. It's not recommended to play on this difficulty!

### Map creation:
I've added the ability to create custom maps. These maps must be of a certain size (height x width) - this is defined in-game by the `mapHeight` and `mapWidth` variables. You will notice
that the `mapHeight` variable is always one more than the `mapWidth` variable, this doesn't _have_ to be the case, however it means you can have an equal height/width map plus the top row
to display the score. Your maps must conform to this size. Here's an example of a map template:

```
#                            #
##############################
#                            #
#                            #
#                            #
#         P <-- Player spawn #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            D  <---- Here's the door to the next level
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
#                            #
##############################
```

Notice the top row, this is where the score is kept. Please don't use the top row as an actual part of the map as you won't be able to see it in-game.

##### Player spawn:
The player spawn point is marked with a `P` on the map.

##### Door to next level:
Once the player has collected all coins on the map, a door should 'open' for them to progress to the next level. This is marked on the map with a `D`, this should be
placed on the edge of the map so it is visible once the player is ready to move on. During the game, this `D` character is replaced with a `#` to make it seem like a regular
wall, but once all coins are collected, this 'wall' disappears, forming a door-way for the player to move onto the next level.