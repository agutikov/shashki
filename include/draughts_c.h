#ifndef __ENGINE__
#define __ENGINE__

// C interface

#include <stdlib.h>
#include <stdint.h>



typedef struct 
{
    uint32_t w_kings;
    uint32_t w_items;

    uint32_t b_kings;
    uint32_t b_items;
} board_t;

board_t get_initial_board();


typedef struct
{
    size_t size;
    board_t* data;
} boards_t;


void free_boards(boards_t* boards);


typedef int (*gen_moves_callback_t)(boards_t* out, board_t b, int turn);


// return >=0 number of generated boards if success
// return <0 if error
int generate_moves(boards_t* out, board_t b, int is_white_turn);


typedef int (*board_callback_t)(board_t b, size_t depth);


int print_board(board_t b, size_t depth);

// b - initial board state
// is_white_turn - !0 if white turn, 0 if black turn
// c - optional, custom user-defined callback, e.g. print_board()
//     if return 0 - stop walkaround
// max_depth - maximum desired depth of walk
//
// return 0 if walkaround completed
// return 1 if stopped by board callback 
int walk_all_moves(board_t b, int is_white_turn, board_callback_t c, unsigned int max_depth);



#endif
