#include <cstdlib>

#include "draughts.h"

extern "C" {
#include "draughts_c.h"



static board_t to_c_board(const board_state_t& b, bool rotate)
{
    if (rotate) {
        return {
            reverse(b.sides[1].kings).mask,
            reverse(b.sides[1].items).mask,
            reverse(b.sides[0].kings).mask,
            reverse(b.sides[0].items).mask,
        };
    } else {
        return {
            b.sides[0].kings.mask,
            b.sides[0].items.mask,
            b.sides[1].kings.mask,
            b.sides[1].items.mask,
        };
    }
}

static board_state_t from_c_board(const board_t& b, bool rotate)
{
    if (rotate) {
        return {
            board_side_t{reverse(b.b_kings), reverse(b.b_items)},
            board_side_t{reverse(b.w_kings), reverse(b.w_items)}
        };
    } else {
        return {
            board_side_t{b.w_kings, b.w_items},
            board_side_t{b.b_kings, b.b_items}
        };
    }
}


board_t get_initial_board()
{
    return to_c_board(initial_board, false);
}

void print_board(board_t b)
{
    print(from_c_board(b, false));
}

void print_tree(board_tree_node_t* t)
{

}

void free_board_tree(board_tree_node_t* tree)
{
    if (!tree || tree->next_states_status <= 0 || !tree->next_states) {
        return;
    }
    for (int i = 0; i < tree->next_states_status; i++) {
        free_board_tree(&tree->next_states[i]);
    }
    free(tree->next_states);
    tree->next_states = 0;
    tree->next_states_status = 0;
}


int verify_move(board_t initial_state, unsigned int from, unsigned int to)
{
    if (from >= 32 || to >= 32) {
        return 0;
    }

    int white_move;
    uint32_t final_items;
    uint32_t from_mask = 1 << from;
    uint32_t to_mask = 1 << to;
    if (to_mask & initial_state.b_items || to_mask & initial_state.w_items) {
        return 0;
    }
    if (from_mask & initial_state.w_items) {
        final_items = initial_state.w_items;
        white_move = 1;
    } else if (from_mask & initial_state.b_items) {
        final_items = initial_state.b_items;
        white_move = 0;
    } else {
        return 0;
    }
    final_items &= ~from_mask;
    final_items |= to_mask;

    board_states_generator g;

    const auto& v = g.gen_next_states(from_c_board(initial_state, !white_move));

    for (const auto& brd : v) {
        board_t b = to_c_board(brd, !white_move);
        if ((white_move && b.w_items == final_items)
        || (!white_move && b.b_items == final_items)) {
            return 1;
        }
    }

    return 0;
}

static unsigned int leftmost_bit(uint32_t mask)
{
    for (unsigned int i = 0; i< 32; i++) {
        if (mask & (1 << i)) {
            return i;
        }
    }
    return 32;
}


board_tree_node_t
generate_item_moves(
    board_t initial_state,
    unsigned int item,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int)
)
{
    board_tree_node_t r{initial_state, -3, 0};

    if (item >= 32) {
        return r;
    }

    int white_move;
    uint32_t from_mask = 1 << item;
    if (from_mask & initial_state.w_items) {
        white_move = 1;
    } else if (from_mask & initial_state.b_items) {
        white_move = 0;
    } else {
        return r;
    }

    board_states_generator g;

    const auto& v = g.gen_item_next_states(from_c_board(initial_state, !white_move), brd_index_t(item));

    if (v.size() == 0) {
        r.next_states_status = 0;
        return r;
    }

    r.next_states = (board_tree_node_t*)calloc(v.size(), sizeof(board_tree_node_t));
    if (!r.next_states) {
        return r;
    }

    r.next_states_status = 0;
    for (const auto& brd : v) {
        auto b = to_c_board(brd, !white_move);
        
        uint32_t to_mask = white_move ? b.w_items & ~initial_state.w_items : b.b_items & ~initial_state.b_items;
        if (to_mask == 0){
            to_mask = from_mask;
        }
        unsigned int to_index = leftmost_bit(to_mask);
        if (to_index >= 32 || !verify_move_callback(initial_state, item, to_index)) {
            continue;
        }

        r.next_states[r.next_states_status].state = b;
        r.next_states[r.next_states_status].next_states_status = -1;
        r.next_states[r.next_states_status].next_states = 0;
        r.next_states_status++;
    }

    return r;
}


board_tree_node_t
generate_all_moves(
    board_t initial_state,
    int white_move,
    int (*)(board_t, unsigned int, unsigned int),
    board_tree_node_t (*generate_item_moves_callback)(
        board_t,
        unsigned int,
        int (*)(board_t, unsigned int, unsigned int)
    ),
    size_t max_depth
)
{
    board_tree_node_t r{initial_state, -3, 0};



    return r;
}


} // extern "C"


