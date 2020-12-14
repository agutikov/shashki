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
 * @brief Prints board state,
 * 
 * @param b board state
 */
void print_board(board_t b);


/**
 * @struct board_tree_node_t
 * @brief Decision tree node.
 * 
 * next_states_status contains the size of next_states array
 * or indicates the reason why there are no next_states
 * next_states_status == 0  - no moves available, one side win
 * next_states_status > 0   - size of next_states array
 * next_states_status == -1 - depth limit
 * next_states_status == -2 - loop detected, next_states points to single node among parent nodes, containing same board state
 * next_states_status == -3 - calculation stopped due to error
 */
typedef struct board_tree_node
{
    board_t state;

    int next_states_status;
    struct board_tree_node* next_states;
} board_tree_node_t;


/**
 * @brief Prints board tree,
 * 
 * @param t board tree
 */
void print_tree(const board_tree_node_t* t);

/**
 * Free memory allocated by board state tree.
 * 
 * If tree == 0 or next_states does not holding memory - do nothing.
 */
void free_board_tree(board_tree_node_t* tree);



/**
 * @brief The function that can determine if a move is valid.
 * 
 * @param initial_state Initial board status.
 * @param from The initial position of the piece to be moved. Must be in range [0, 31].
 * @param to The final position of the piece to be moved. Must be in range [0, 31].
 * 
 * @return 1 - if from and to define correct possible move, 0 - otherwise
 */
int verify_move(board_t initial_state, unsigned int from, unsigned int to);


/**
 * @brief The function that generates the possible moves for a piece.
 * 
 * @param initial_state Initial board status.
 * @param item The initial piece position.
 * @param verify_move_callback A callable that can determine if a move is valid.
 * 
 * @return Resulting decision tree with max depth=1. 
 */
board_tree_node_t
generate_item_moves(
    board_t initial_state,
    unsigned int item,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int)
);


/**
 * @brief The function that generates all the possible moves that can be played up to a specified number of moves
 * 
 * Function performs Depth-first search and collects results into output tree.
 * 
 * @param initial_state Initial board status.
 * @param white_move The player that makes the first move. 0 - black, !0 - white.
 * @param verify_move_callback A callable that generates the possible moves for a piece.
 * @param generate_item_moves_callback A callable that can determine if a move is valid.
 * @param max_depth The desired maximum depth of moves.
 * 
 * @return Resulting decision tree.
 */
board_tree_node_t
generate_all_moves(
    board_t initial_state,
    int white_move,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int),
    board_tree_node_t (*generate_item_moves_callback)(
        board_t,
        unsigned int,
        int (*)(board_t, unsigned int, unsigned int)
    ),
    size_t max_depth
);





#endif
