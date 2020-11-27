#ifndef __ENGINE__
#define __ENGINE__

// C interface

#include <stdlib.h>
#include <stdint.h>


/**
 * Structure representing board state.
 * 
 * All fields are bitmaps.
 * Simple items exist only on w_items or b_items.
 * Kings are also exists on w_kings and b_kings.
 */
typedef struct 
{
    uint32_t w_kings;
    uint32_t w_items;

    uint32_t b_kings;
    uint32_t b_items;
} board_t;

/**
 * Function returns initial board state according to Russian draughts rules.
 */
board_t get_initial_board();


/**
 * Structure contains multiple boards.
 * 
 * Produced by generate_moves().
 * Freed by free_boards().
 */
typedef struct
{
    size_t size;
    board_t* data;
} boards_t;


/**
 * Free memory allocated by boards in boards_t.
 * 
 * If boards == 0 or boards does not holding memory - do nothing.
 */
void free_boards(boards_t* boards);

/**
 * Function generates set of possible subsequent boards.
 * Function allocates memory for resulting data.
 * 
 * @param out boards buffer for result
 * @param b initial board state
 * @param is_white_turn
 * 
 * @return >=0 number of generated boards if success, <0 if error
 */
int generate_moves(boards_t* out, board_t b, int is_white_turn);


/**
 * Callback type for walking board state tree.
 * @see walk_all_moves()
 */
typedef int (*board_callback_t)(board_t b, size_t depth);

/**
 * @brief Prints board state,
 * 
 * @param b board state
 * @param depth current depth
 * @return int always returns 1
 */
int print_board(board_t b, size_t depth);


/**
 * @brief Perform DFS walkthrough of possible board states
 *        starting from given initial state
 *        up to provided depth.
 * 
 * If callback provided than it will be called for each generated board state.
 * If callbeck returns 0 - walkthough stops.
 * 
 * @param b initial board state
 * @param is_white_turn flag indicating next move is done by white party
 * @param c optional callback
 * @param max_depth 
 * @return int 0 if walkthough completed, 1 if teminated.
 */
int walk_all_moves(board_t b, int is_white_turn, board_callback_t c, unsigned int max_depth);



#endif
