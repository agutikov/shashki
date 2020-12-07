
#include "draughts_c.h"


int main()
{
    board_tree_node_t tree = generate_all_moves(get_initial_board(), 1, generate_item_moves, verify_move, 100);

    print_tree(&tree);


    return 0;
}

