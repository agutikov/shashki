#include "dfs.h"

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

static boards_t to_c_boards(const std::vector<board_state_t>& b, bool rotate)
{
    boards_t r{0, 0};

    if (b.size() == 0) {
        return r;
    }

    r.data = (board_t*) calloc(b.size(), sizeof(*r.data));
    if (r.data == 0) {
        return r;
    }

    for (const auto& brd : b) {
        r.data[r.size] = to_c_board(brd, rotate);
        r.size++;
    }

    return r;
}



void free_boards(boards_t* boards)
{
    if (!boards || !boards->data) {
        return;
    }
    free(boards->data);
    boards->data = 0;
    boards->size = 0;
}


int generate_moves(boards_t* out, board_t b, int is_white_turn)
{
    if (!out) {
        return -1;
    }

    board_state_t brd = from_c_board(b, !is_white_turn);

    board_states_generator g;

    auto& v = g.gen_next_states(brd);

    *out = to_c_boards(v, !is_white_turn);

    return out->size;
}


int print_board(board_t b, size_t depth)
{
    printf("depth: %lu\n", depth);
    print(from_c_board(b, false));

    return 1;
}

board_t get_initial_board()
{
    return to_c_board(initial_board, false);
}

int walk_all_moves(board_t b, int is_white_turn, board_callback_t c, unsigned int max_depth)
{
    search_config_t scfg{
        max_depth,
        Clock::now() + 3h,
        0,
        false,
        false
    };

    brd_callback_t callback;
    if (c) {
        callback = [c, is_white_turn] (const board_state_t& brd, size_t depth) -> bool {
            return c(to_c_board(brd, !is_white_turn), depth);
        };
    }

    DFS<judy_cache> dfs(scfg, false, false, false, callback);

    auto r = dfs._do_search(from_c_board(b, !is_white_turn));

    return std::get<1>(r) ? 0 : 1;
}




} // extern "C"


