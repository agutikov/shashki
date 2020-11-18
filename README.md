

# The Problem

1. Build a game engine library that fulfills the following specifications:
    1.1.- Builds on Linux.
    1.2.- Can be built using your preferred build system (make, cmake, meson, etc...).
    1.2.- Has a function that generates all the possible moves that can be played up to a specified number of moves.
        1.2.2.- This function should take as arguments:
            - Initial board status.
            - The player that makes the first move.
            - A callable that can determine if a move is valid.
            - A callable that generates the possible moves for a piece.
            - The desired maximum depth of moves.
        1.2.3.- The callable that can determine if a move is valid should take the following arguments:
            - Initial board status.
            - The initial position of the piece to be moved.
            - The final position of the piece to be moved.
        1.2.4.- The callable that generates the possible moves for a piece should take the following arguments:
            - Initial board status.
            - The initial piece position
            - The type of piece
            - A callable that can determine if a move is valid.
    1.3.- Write an example that generates the decision tree for the following configuration:
        - Maximum tree depth is 100 moves.
        - The White player is the first to move.
        - Use the Russian draughts rules as specified below to write your callables.
        - Use the initial Russian draughts board as the initial board status.
2. Write a C API to use the engine library
3. Add unit tests for the C API
4. Multithread the generation of possible moves (optional)

Additional game details:
    Board description: 8×8 board with alternating dark and light squares. The left down square field should be dark.
    Starting position: Each player starts with 12 pieces on the three rows closest to their own side, pieces are located in the dark squares. The row closest to each player is called the "crownhead" or "kings row". Usually, the colors of the pieces are black and white, but possible use other colors (one dark and other light).
    Player turns: The player with white pieces (lighter color) moves first.
    Pieces and their details:
    Kings:
    - Are differentiated as consisting of two normal pieces of the same color, stacked one on top of the other or by inverted pieces.
    - If a player's piece moves into the kings row on the opposing player's side of the board, that piece to be "crowned", becoming a "king" and gaining the ability to move back or forward and choose on which free square at this diagonal to stop.
    Men: Men move forward diagonally to an adjacent unoccupied square.
    Actions:
    Capture:
    - If the adjacent square contains an opponent's piece, and the square immediately beyond it is vacant, the opponent's piece may be captured (and removed from the game) by jumping over it. Jumping can be done forward and backward. Multiple-jump moves are possible if, when the jumping piece lands, there is another piece that can be jumped. Jumping is mandatory and cannot be passed up to make a non-jumping move. When there is more than one way for a player to jump, one may choose which sequence to make, not necessarily the sequence that will result in the most amount of captures. However, one must make all the captures in that sequence. A captured piece is left on the board until all captures in a sequence have been made but cannot be jumped again (this rule also applies for the kings).
    - If a man touches the kings row during a capture and can continue a capture, it jumps backwards as a king. The player can choose where to land after the capture.
    Winning and draws:

A player with no valid move remaining loses. This is the case if the player either has no pieces left or if a player's pieces are obstructed from making a legal move by the pieces of the opponent. A game is a draw if neither opponent has the possibility to win the game. The game is considered a draw when the same position repeats itself for the third time, with the same player having the move each time. If one player proposes a draw and his opponent accepts the offer. If a player has three kings (or more) in the game against a single enemy king and his 15th move (counting from the time of establishing the correlation of forces) cannot capture enemy king.



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




