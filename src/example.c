
#include "draughts_c.h"


int main()
{
    board_tree_node_t tree = generate_all_moves(get_initial_board(), 1, verify_move, generate_item_moves, 10);

    print_tree(&tree);

    free_board_tree(&tree);


    return 0;
}

