#include <cstdlib>
#include <iostream>
#include <map>

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


static void _print_tree(const board_tree_node_t* t, size_t depth)
{
    if (!t) {
        return;
    }

    printf("\nMove %lu:\n", depth);
    print_board(t->state);

    depth++;

    switch(t->next_states_status) {
        case BOARD_TREE_STATUS_ERROR:
            printf("Move %lu: ERROR\n", depth);
            return;
        case BOARD_TREE_STATUS_LOOP:
            printf("Move %lu: Loop\n", depth);
            t = t->next_states;
            for (int i = 0; i < t->next_states_status; i++) {
                printf("\nMove %lu (loop):\n", depth);
                print_board(t->next_states[i].state);
            }
            return;
        case BOARD_TREE_STATUS_DEPTH:
            //printf("Move %lu: Depth limit\n", depth);
            return;
        case 0:
            printf("Move %lu: Game over\n", depth);
            return;
    };

    if (!t->next_states) {
        printf("Move %lu: ERROR: next_states == 0\n", depth);
        return;
    }

    for (int i = 0; i < t->next_states_status; i++) {
        _print_tree(&t->next_states[i], depth);
    }
}


void print_tree(const board_tree_node_t* t)
{
    _print_tree(t, 0);
}


/**
 * WARNING! Function returns non-initialized memory under board_tree_node_t::next_states.
 */
static board_tree_node_t new_board_tree_node(const board_t& b, int state_size)
{
    board_tree_node_t r{b, state_size, 0};

    if (state_size <= 0) {
        return r;
    }

    r.next_states = (board_tree_node_t*) calloc(state_size, sizeof(board_tree_node_t));

    if (r.next_states == 0) {
        r.next_states_status = BOARD_TREE_STATUS_ERROR;
    }

    return r;
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


static std::pair<board_t, std::vector<board_t>> from_c_tree_node(const board_tree_node_t& t)
{
    if (t.next_states_status <= 0) {
        return {t.state, {}};
    }
    std::vector<board_t> v;

    for (int i = 0; i < t.next_states_status; i++) {
        v.push_back(t.next_states[i].state);
    }

    return {t.state, v};
}


static board_tree_node_t to_c_tree_node(const std::pair<board_t, std::vector<board_t>>& t)
{
    board_tree_node_t r = new_board_tree_node(t.first, t.second.size());

    if (r.next_states_status <= 0) {
        return r;
    }

    for (int i = 0; i < r.next_states_status; i++) {
        r.next_states[i] = new_board_tree_node(t.second[i], 0);
    }

    return r;
}


static brd_index_t from_c_index(unsigned int index, bool rotate)
{
    return rotate ? brd_index_t(31 - index) : brd_index_t(index);
}


static int _verify_move(board_t initial_state, unsigned int from, unsigned int to) noexcept
{
    try {
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
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}


int verify_move(board_t initial_state, unsigned int from, unsigned int to)
{
    return _verify_move(initial_state, from, to);
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


static board_tree_node_t
_generate_item_moves(
    board_t initial_state,
    unsigned int item_index,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int)
) noexcept
{
    board_tree_node_t r{initial_state, BOARD_TREE_STATUS_ERROR, 0};

    if (item_index >= 32) {
        return r;
    }

    int white_move;
    uint32_t from_mask = 1 << item_index;
    if (from_mask & initial_state.w_items) {
        white_move = 1;
    } else if (from_mask & initial_state.b_items) {
        white_move = 0;
    } else {
        return r;
    }

    try {
        board_states_generator g;

        const auto& v = g.gen_item_next_states(from_c_board(initial_state, !white_move), from_c_index(item_index, !white_move));

        if (v.size() == 0) {
            r.next_states_status = 0;
            return r;
        }

        r = new_board_tree_node(initial_state, v.size());
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
            if (to_index >= 32 || !verify_move_callback(initial_state, item_index, to_index)) {
                continue;
            }

            r.next_states[r.next_states_status].state = b;
            r.next_states[r.next_states_status].next_states_status = BOARD_TREE_STATUS_DEPTH;
            r.next_states[r.next_states_status].next_states = 0;
            r.next_states_status++;
        }

        return r;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return {initial_state, BOARD_TREE_STATUS_ERROR, 0};
    }
}


board_tree_node_t
generate_item_moves(
    board_t initial_state,
    unsigned int item_index,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int)
)
{
    return _generate_item_moves(initial_state, item_index, verify_move_callback);
}


static board_tree_node_t
_generate_all_moves_lvl(
    board_t initial_state,
    int white_move,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int),
    board_tree_node_t (*generate_item_moves_callback)(
        board_t,
        unsigned int,
        int (*)(board_t, unsigned int, unsigned int)
    )
)
{
    std::pair<board_t, std::vector<board_t>> r;

    r.first = initial_state;

    for (unsigned int index = 0; index < 32; index++) {
        bool white_exist = initial_state.w_items & (1 << index);
        bool black_exist = initial_state.b_items & (1 << index);
        if (!(white_move && white_exist) && !(!white_move && black_exist)) {
            continue;
        }
        board_tree_node_t node = generate_item_moves_callback(initial_state, index, verify_move_callback);
        if (node.next_states_status > 0) {
            std::pair<board_t, std::vector<board_t>> items = from_c_tree_node(node);
            r.second.reserve(r.second.size() + items.second.size());
            r.second.insert(r.second.end(), items.second.begin(), items.second.end());
        } else if (node.next_states_status == BOARD_TREE_STATUS_ERROR) {
            return {initial_state, BOARD_TREE_STATUS_ERROR, 0};
        }
    }

    return to_c_tree_node(r); 
}


struct board_cmp
{
    bool operator()(const board_t& lhs, const board_t& rhs) const
    { 
        return lhs.w_kings < rhs.w_kings
            || lhs.b_kings < rhs.b_kings
            || lhs.w_items < rhs.w_items
            || lhs.b_items < rhs.b_items
        ;
    }
};

typedef std::map<board_t, board_tree_node_t*, board_cmp> stack_map_t;

static board_tree_node_t
_generate_all_moves_r(
    stack_map_t& stack_map,
    board_t initial_state,
    int white_move,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int),
    board_tree_node_t (*generate_item_moves_callback)(
        board_t,
        unsigned int,
        int (*)(board_t, unsigned int, unsigned int)
    ),
    size_t max_depth
)
{
    if (max_depth == 0) {
        return new_board_tree_node(initial_state, BOARD_TREE_STATUS_DEPTH);
    }

    board_tree_node_t root = _generate_all_moves_lvl(initial_state, white_move, verify_move_callback, generate_item_moves_callback);
    if (root.next_states_status <= 0) {
        return root;
    }

    max_depth--;
    for (int i = 0; i < root.next_states_status; i++) {
        auto p = stack_map.emplace(root.next_states[i].state, &root.next_states[i]);
        if (!p.second) {
            root.next_states[i] = new_board_tree_node(root.next_states[i].state, BOARD_TREE_STATUS_LOOP);
            root.next_states[i].next_states = p.first->second;
            continue;
        }

        root.next_states[i] =
        _generate_all_moves_r(
            stack_map,
            root.next_states[i].state,
            !white_move,
            verify_move_callback,
            generate_item_moves_callback,
            max_depth
        );

        stack_map.erase(p.first);
    }

    return root;
}


static board_tree_node_t
_generate_all_moves(
    board_t initial_state,
    int white_move,
    int (*verify_move_callback)(board_t, unsigned int, unsigned int),
    board_tree_node_t (*generate_item_moves_callback)(
        board_t,
        unsigned int,
        int (*)(board_t, unsigned int, unsigned int)
    ),
    size_t max_depth
) noexcept
{
    try {
        if (!generate_item_moves_callback) {
            return new_board_tree_node(initial_state, BOARD_TREE_STATUS_ERROR);
        }

        stack_map_t stack_map;

        return _generate_all_moves_r(stack_map, initial_state, white_move, verify_move_callback, generate_item_moves_callback, max_depth);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return new_board_tree_node(initial_state, BOARD_TREE_STATUS_ERROR);
    }
}


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
)
{
    return _generate_all_moves(initial_state, white_move, verify_move_callback, generate_item_moves_callback, max_depth);
}


} // extern "C"


