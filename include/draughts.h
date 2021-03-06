#pragma once

#include <cstdio>
#include <array>
#include <utility>
#include <vector>
#include <algorithm>
#include <string>

#include "draughts_tables.h"

using namespace std::string_literals;


struct board_side_t
{
    brd_map_t kings;
    brd_map_t items;

    explicit operator uint64_t() const
    {
        return uint64_t(kings.mask) << 32 | items.mask;
    }

    friend bool operator==(const board_side_t& lhs, const board_side_t& rhs)
    {
        return lhs.kings == rhs.kings && lhs.items == rhs.items;
    }

    void bit_add(const board_side_t& b)
    {
        kings += b.kings;
        items += b.items;
    }

    bool contains(const board_side_t& b)
    {
        return kings.exist_all(b.kings) && items.exist_all(b.items);
    }
};

struct board_state_t
{
    std::array<board_side_t, 2> sides;

    explicit operator std::pair<uint64_t, uint64_t>() const
    {
        //return {uint64_t(sides[0]), uint64_t(sides[1])};
        return {
            (uint64_t(sides[0].kings.mask) << 32) | sides[1].kings.mask,
            (uint64_t(sides[0].items.mask) << 32) | sides[1].items.mask
        };
    }

    brd_map_t occupied() const
    {
        return sides[0].items + sides[1].items;
    }

    friend bool operator==(const board_state_t& lhs, const board_state_t& rhs)
    {
        return lhs.sides[0] == rhs.sides[0] && lhs.sides[1] == rhs.sides[1];
    }

    void bit_add(const board_state_t& b)
    {
        sides[0].bit_add(b.sides[0]);
        sides[1].bit_add(b.sides[1]);
    }

    bool contains(const board_state_t& b)
    {
        return sides[0].contains(b.sides[0]) && sides[1].contains(b.sides[1]);
    }
};

inline const board_state_t initial_board = {board_side_t{0, 0x0FFF}, board_side_t{0, 0xFFF00000}};

inline const brd_index_t format_table[8][8] = {
    {{}, 28, {}, 29, {}, 30, {}, 31},
    {24, {}, 25, {}, 26, {}, 27, {}},
    {{}, 20, {}, 21, {}, 22, {}, 23},
    {16, {}, 17, {}, 18, {}, 19, {}},
    {{}, 12, {}, 13, {}, 14, {}, 15},
    { 8, {},  9, {}, 10, {}, 11, {}},
    {{},  4, {},  5, {},  6, {},  7},
    { 0, {},  1, {},  2, {},  3, {}},
};

void print(const board_state_t& s)
{
    const char* cols = "    a   b   c   d   e   f   g   h";
    const char* line = "  +---+---+---+---+---+---+---+---+";
    size_t row = 8;
    printf("%s\n", line);
    for (const auto& f : format_table) {
        printf("%lu |", row);
        for (brd_index_t index : f) {
            if (!index) {
                printf("   |");
            } else {
                auto item = brd_item_t(index);
                const char* c = s.sides[0].kings.exist(item) ? " @ " :
                         s.sides[0].items.exist(item) ? " o " :
                         s.sides[1].kings.exist(item) ? " # " :
                         s.sides[1].items.exist(item) ? " x " : "   ";
                printf("%s|", c);
            }
        }
        printf("\n%s\n", line);
        row--;
    }
    printf("%s\n", cols);
}


std::ostream& operator<< (std::ostream& out, const board_state_t& brd)
{
    auto cols = "    a   b   c   d   e   f   g   h"s;
    auto line = "  +---+---+---+---+---+---+---+---+"s;
    size_t row = 8;
    out << line << "\n"s;
    for (const auto& f : format_table) {
        out << std::to_string(row) << " |"s;
        for (brd_index_t index : f) {
            if (!index) {
                out << "   |"s;
            } else {
                auto item = brd_item_t(index);
                auto c = brd.sides[0].kings.exist(item) ? " @ "s :
                         brd.sides[0].items.exist(item) ? " o "s :
                         brd.sides[1].kings.exist(item) ? " # "s :
                         brd.sides[1].items.exist(item) ? " x "s : "   "s;
                out << c << "|"s;
            }
        }
        out << "\n"s << line << "\n"s;
        row--;
    }
    out << cols << "\n\n"s;
    return out;
}

board_side_t reverse(const board_side_t& b)
{
    return {reverse(b.kings), reverse(b.items)};
}

board_state_t rotate(const board_state_t& s)
{
    return {reverse(s.sides[1]), reverse(s.sides[0])};
}
//TODO: benchmark rotation


board_state_t do_move(board_state_t state, brd_item_t src, brd_item_t dst)
{
    state.sides[0].items -= src;
    state.sides[0].items += dst;

    if (state.sides[0].kings.exist(src)) {
        state.sides[0].kings -= src;
        state.sides[0].kings += dst;
    }

    // become new king
    if (dst.is_on_king_row()) {
        state.sides[0].kings += dst;
    }

    return state;
}

board_state_t do_capture(board_state_t state, brd_map_t capture)
{
    state.sides[1].items -= capture;
    if (state.sides[1].kings.exist_any(capture)) {
        state.sides[1].kings -= capture;
    }
    return state;
}





struct _board_states_generator
{
    _board_states_generator(std::vector<board_state_t>& buffer) :
        _states(buffer)
    {}

    _board_states_generator(const _board_states_generator&) = delete;
    _board_states_generator(_board_states_generator&&) = delete;
    _board_states_generator& operator=(const _board_states_generator&) = delete;
    _board_states_generator& operator=(_board_states_generator&&) = delete;

    size_t gen_item_captures(brd_index_t item_pos)
    {
        const board_side_t& player = cur_state.sides[0];
        auto item = brd_item_t(item_pos);

        if (player.kings.exist(item)) {
            next_king_captures(cur_state, item_pos, {});
        } else if (player.items.exist(item)) {
            next_item_captures(cur_state, item_pos, {});
        }

        return generated;
    }

    size_t gen_captures()
    {
        for (brd_index_t pos = 0; pos; ++pos) {
            gen_item_captures(pos);
        }

        return generated;
    }

    size_t gen_item_moves(brd_index_t item_pos)
    {
        auto item = brd_item_t(item_pos);

        const board_side_t& player = cur_state.sides[0];
        if (player.kings.exist(item)) {
            next_king_moves(item_pos);
        } else if (player.items.exist(item)) {
            next_item_moves(item_pos);
        }

        return generated;
    }

    size_t gen_moves()
    {
        for (brd_index_t item_pos = 0; item_pos; ++item_pos) {
            gen_item_moves(item_pos);
        }

        return generated;
    }

    void prepare(const board_state_t& brd)
    {
        cur_state = brd;
        occupied = cur_state.occupied();
        bit_filter = board_state_t{board_side_t{0, 0}, board_side_t{0, 0}};
        generated = 0;
    }

    size_t gen_next_states(const board_state_t& brd)
    {
        //TOOD: Refactor, 30 Mboards/s

        prepare(brd);

        // As capture is mandatory and can't be skipped
        // So first - try capture, and if no available captures - then try move
        if (gen_captures()) {
            return generated;
        }

        return gen_moves();
    }


    // move active item between recursive calls (change state) and collect captured enemies
    // when capture sequence completed - remove captured enemies before state saved
    size_t next_item_captures(const board_state_t& state, brd_index_t item_pos, brd_map_t captured)
    {
        // Rules:
        // - enemy items have to be removed after capture chain finished completely
        // - every enemy item can't be captured twice
        brd_map_t available_dst = tables.capture_move_masks[item_pos.index] - state.occupied();
        brd_map_t may_be_captured = state.sides[1].items.select(tables.capture_masks[item_pos.index]);

        // not allow capture same items multiple times
        may_be_captured -= captured;

        size_t saved_states = 0;
        if (may_be_captured && available_dst) {
            for (auto [capture_item, dst_index] : tables.captures[item_pos.index]) {
                auto dst = brd_item_t(dst_index);
                if (!dst) {
                    break;
                }
                // branching possible capture moves, follow moves
                if (may_be_captured.exist(capture_item) && available_dst.exist(dst)) {
                    // do move
                    board_state_t next_state = do_move(state, brd_item_t(item_pos), dst);

                    // try continue capturing
                    saved_states += next_item_captures(next_state, dst_index, captured + capture_item);
                }
            }
        }

        // nothing more can be captured
        if (saved_states == 0) {
            // capture sequence completed
            if (captured) {
                // do capture
                board_state_t next_state = do_capture(state, captured);

                bool duplicate = false;
                if (bit_filter.contains(next_state)) {
                    for (const auto& b : _states) {
                        if (b == next_state) {
                            duplicate = true;
                            break;
                        }
                    }
                }

                if (!duplicate) {
                    // save new state if it is final
                    _states.push_back(next_state);
                    bit_filter.bit_add(next_state);
                    generated++;
                }
                return 1;
            } else {
                return 0;
            }
        }

        return saved_states;
    }

    void next_item_moves(brd_index_t item_pos)
    {
        brd_map_t available_dst = tables.fwd_dst_masks[item_pos.index] - occupied;

        if (!available_dst) {
            return;
        }

        for (brd_item_t dst : tables.fwd_destinations[item_pos.index]) {
            if (!dst) {
                break;
            }
            if (available_dst.exist(dst)) {
                // do move - get new board state
                board_state_t next_state = do_move(cur_state, brd_item_t(item_pos), dst);
                // save new state
                _states.push_back(next_state);
                generated++;
            }
        }
    }

    //TODO: Generalize items and kings tables and next_* functions, parametrize function template with const table

    size_t next_king_captures(const board_state_t& state, brd_index_t item_pos, brd_map_t captured)
    {
        brd_map_t cur_occupied = state.occupied();
        brd_map_t available_dst = tables.king_capture_move_masks[item_pos.index] - cur_occupied;
        brd_map_t may_be_captured = state.sides[1].items.select(tables.king_capture_masks[item_pos.index]);

        // not allow capture same items multiple times
        may_be_captured -= captured;

        size_t saved_states = 0;
        if (may_be_captured && available_dst) {
            // iter directions
            for (const auto& dir_captures : tables.king_captures[item_pos.index]) {
                // iter captures in direction
                for (const auto& dir_capture : dir_captures) {
                    brd_item_t capture_item = dir_capture.first;

                    if (!capture_item) {
                        break;
                    }
                    // cant't jump over allies
                    if (state.sides[0].items.exist(capture_item)) {
                        break;
                    }

                    if (may_be_captured.exist(capture_item)) {
                        // iter over possible jump destinations
                        for (brd_index_t dst_index : dir_capture.second) {
                            if (!dst_index) {
                                break;
                            }
                            auto dst = brd_item_t(dst_index);

                            // cant't jump over allies or capture/jump over multiple enemy items
                            if (cur_occupied.exist(dst)) {
                                break;
                            }

                            if (available_dst.exist(dst)) {
                                // do move
                                board_state_t next_state = do_move(state, brd_item_t(item_pos), dst);

                                // try continue capturing
                                saved_states += next_item_captures(next_state, dst_index, captured + capture_item);
                            }
                        }

                        // cant't jump over enemies without capture
                        break;
                    }
                }
            }
        }

        // nothing more can be captured
        if (saved_states == 0) {
            // capture sequence completed
            if (captured) {
                // do capture
                board_state_t next_state = do_capture(state, captured);

                bool duplicate = false;
                if (bit_filter.contains(next_state)) {
                    for (const auto& b : _states) {
                        if (b == next_state) {
                            duplicate = true;
                            break;
                        }
                    }
                }

                if (!duplicate) {
                    // save new state if it is final
                    _states.push_back(next_state);
                    bit_filter.bit_add(next_state);
                    generated++;
                }

                return 1;
            } else {
                return 0;
            }
        }

        return saved_states;
    }

    void next_king_moves(brd_index_t item_pos)
    {
        brd_map_t available_dst = tables.king_move_masks[item_pos.index] - occupied;

        if (!available_dst) {
            return;
        }

        for (const auto& move_dir : tables.king_moves[item_pos.index]) {
            for (brd_item_t dst : move_dir) {
                if (!dst) {
                    break;
                }
                if (available_dst.exist(dst)) {
                    // do move - get new board state
                    board_state_t next_state = do_move(cur_state, brd_item_t(item_pos), dst);
                    // save new state
                    _states.push_back(next_state);
                    generated++;
                } else {
                    // capture handled in next_king_captures()
                    // can't jump while move without capture
                    break;
                }
            }
        }
    }


    brd_map_t occupied;
    board_state_t cur_state;

    std::vector<board_state_t>& _states;
    size_t generated;
    board_state_t bit_filter;
};


// empirical 
#define MAX_LEVEL_WIDTH 64

struct board_states_generator
{
    board_states_generator() :
        states(MAX_LEVEL_WIDTH),
        g(states)
    {}
    
    board_states_generator(board_states_generator&& other) :
        states(std::move(other.states)),
        g(states)
    {}

    board_states_generator(const board_states_generator& other) :
        states(other.states),
        g(states)
    {}

    board_states_generator& operator=(board_states_generator&&) = delete;
    board_states_generator& operator=(const board_states_generator&) = delete;

    const std::vector<board_state_t>& gen_next_states(const board_state_t& brd)
    {
        states.clear();
        g.gen_next_states(brd);
        return states;
    }

    const std::vector<board_state_t>& gen_item_next_states(const board_state_t& brd, brd_index_t item_pos)
    {
        states.clear();

        g.prepare(brd);

        if (g.gen_captures()) {
            states.clear();
            g.prepare(brd);
            g.gen_item_captures(item_pos);
        } else {
            g.gen_item_moves(item_pos);
        }

        return states;
    }

private:
    std::vector<board_state_t> states;

    _board_states_generator g;
};
