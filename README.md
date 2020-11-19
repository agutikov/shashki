

# The Problem

1. Build a game engine library that fulfills the following specifications:
   1. Builds on Linux.
      1. Can be built using your preferred build system (make, cmake, meson, etc...).
   2. Has a function that generates all the possible moves that can be played up to a specified number of moves.
      1. This function should take as arguments:
         - Initial board status.
         - The player that makes the first move.
         - A callable that can determine if a move is valid.
         - A callable that generates the possible moves for a piece.
         - The desired maximum depth of moves.
      1. The callable that can determine if a move is valid should take the following arguments:
         - Initial board status.
         - The initial position of the piece to be moved.
         - The final position of the piece to be moved.
      1. The callable that generates the possible moves for a piece should take the following arguments:
         - Initial board status.
         - The initial piece position
         - The type of piece
         - A callable that can determine if a move is valid.
   3. Write an example that generates the decision tree for the following configuration:
   - Maximum tree depth is 100 moves.
   - The White player is the first to move.
   - Use the Russian draughts rules as specified below to write your callables.
   - Use the initial Russian draughts board as the initial board status.
2. Write a C API to use the engine library
3. Add unit tests for the C API
4. Multithread the generation of possible moves (optional)


Russian draughts rules: https://en.wikipedia.org/wiki/Russian_draughts#Rules



# Analisys and Design


The task has an uncertainty if we have both:
- A callable that generates the possible moves for a piece.
- A callable that can determine if a move is valid.

While we have a move validator,
then what first callable should generate exactly?
If we treat move as actions made by one player before turn goes to the other player, 
then there are multiple options for "possible" moves to generate:
a) moves going out of the board;
b) moves on fields occupated by enemies;
c) moves on fields occupated by allies;
d) moves that capture one enemy;
e) moves that capture multiple enemies (according the rules that would be a single move);

Which of them should be filtered during generation and which during validation and why?
One of the most obviously not-optimal way would be to generate a list of all squares of the board
and then try to verify if the given piece can get there by capturing multiple enemy pieces.

I mean that "possible" (according to the rules) moves are the same as "valid" ones.

So seems that move validator is not required.

The possible solution with two callbacks could be:
- A callable that generates the possible straight-line moves for a piece, including possible capturing at most a single enemy.
- A callable that determines if the turn is completed on this step, if next capture is possible or not.

But this implementation would be also not-optimal, because of duplicated checks if capturing possibility.

As I understand the aim of requirement is to distinguish game logic implementatin and search algorythm.

So I propose another set of methods (callbacks):
- Get all possible moves for all possible items, i.e. get all possible states after any item been moved;
- Turn around the board;

While there are always two sides in game and board is symmetric and rules are symmetric
then there is a possibility to avoid handling of "active player"
and replace it with board rotation function.
And just track the color of current "active" side.
It seems more useful, easy to implement and convinient in general.


## Search

In absence of any requirement of desired result except calculate as much moves as possible,
there are two available ways to do it:
- Depth-first search with limited depth
- Breadth-first search

Additionally I see an option of cache the mapping of board states to set of consequent board states.
There can be:
- no cache
- dedicateed cache for each thread
- global cache

I plan to implement as many variants as possible and then compare the results.

DFS should have cycle detection and/or depth limit.
BFS depth is limited by memory.

DFS is preferred for performance benchmarking of state-calculating algorythm, while it's memory consumption is low.


## Unit-tests

We can define rules in two different coordinate systems:
- 2-dimentional, like Chess coordinate system;
- 1-dimentional, where usable (black) squares have one-dimentional end-to-end numbering.

Game engine implemented in 1-dim system.
But rules in first coordinate system are easier to formulate and verify manually.

So Unit-testing can be implemented this way:
1. define initial state in 2-dim system;
2. convert into 1-dim;
3. do calculations with engine - generate possible subsequent states;
4. convert result into 2-dim;
5. verify result in 2-dim.

Compliance with the rules in one coordinate system implies compliance in the other one.

### Correctness and completeness

Tests must verify two independant statements:
- engine generates valid subsequent states;
- engine generates all possible valid subsequent states.

How?

The only way I see is to generate states in 2-dim system and compare with engine results.

That means to implement engine in both coordinate systems.

Question:
While 1-dim engine is implemented with tables generated from 2-dim representation.
And tables are verified manually.
Then probably testing is not required at all.
How 1-dim engine can be implemented from 2-dim rules to eliminate the requirement of testing?


### Testing of conversion itself (between 2-dim and 1-dim representation)

Conversion between 2-dim and 1-dim must be isomorphic.

So it can be tested next way:
1. generate state in 2-dim;
2. convert into 1-dim;
3. convert into 2-dim;
4. compare with initial state.

Complete set of states can't be generated.

Possible set of states:
- empty state;
- one item in every position;
  - total 32 states;
- all items exist in any position (12+12);
  - total = (number of 32-bit itegers with 24 bits set) * (number of 24 bits integers with 12 bits set).

I have no idea if it is possible to guarantee conversion is isomorphic for all states.


### SMT

TODO: Investigate if SMT-solver can be used to verify compliance of the rules by the implemented engine.

While writing SMT rules is complicated itself, it is significantly more similar to defining rules in natural language,
and therefore less error-prone.

#### Dreams

* Stage 1 - Write unit-tests that use SMT-solver to verify results (data);
* Stage 2 - Verify engine code itself;
* Stage 3 - Synthise the entire engine from the SMT rules;
* Stage 4 - PROFIT.


