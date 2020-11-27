
#include "draughts_c.h"


int main()
{
    walk_all_moves(get_initial_board(), 1, print_board, 1);
    walk_all_moves(get_initial_board(), 1, print_board, 2);
    walk_all_moves(get_initial_board(), 1, print_board, 3);
    walk_all_moves(get_initial_board(), 1, print_board, 100);

    return 0;
}

