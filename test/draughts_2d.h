#pragma once

#include <vector>
#include <iostream>

#include "draughts.h"

extern "C" {
#include "draughts_c.h"
}


enum board_cell_state_enum : char
{
    _ = ' ',
    o = 'o',
    G = 'G',
    x = 'x',
    M = 'M'
};

inline board_cell_state_enum available_items[4] = {o, G, x, M};

bool is_valid(board_cell_state_enum state)
{
    return state == _ || state == o || state == G || state == x || state == M;
}

bool is_occupied(board_cell_state_enum state)
{
    return state != _;
}

bool is_black(board_cell_state_enum state)
{
    return state == x || state == M;
}

bool is_white(board_cell_state_enum state)
{
    return state == o || state == G;
}

bool is_king(board_cell_state_enum state)
{
    return state == G || state == M;
}

typedef board_cell_state_enum board_state_2d_t[8][8];

inline const board_state_2d_t valid_positions = {
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _}
};

bool is_valid_pos(size_t row, size_t col)
{
    return row < 8 && col < 8 && (valid_positions[row][col] != _);
}

bool is_white_king_row(size_t row)
{
    return row == 0;
}

bool is_black_king_row(size_t row)
{
    return row == 7;
}

struct board_2d_t
{
    // total number of items and kings (for additional verificaton)
    size_t w_items;
    size_t w_kings;
    size_t b_items;
    size_t b_kings;
    board_state_2d_t state;

    friend bool operator==(const board_2d_t& lhs, const board_2d_t& rhs)
    {
        if (lhs.w_kings != rhs.w_kings ||
            lhs.w_items != rhs.w_items ||
            lhs.b_kings != rhs.b_kings ||
            lhs.b_items != rhs.b_items)
        {
            return false;
        }
        for (size_t row = 0; row < 8; row++) {
            for (size_t col = 0; col < 8; col++) {
                if (lhs.state[row][col] != rhs.state[row][col]) { 
                    return false;
                }
            }
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const board_2d_t& brd)
    {
        os << "w: " << brd.w_kings << "/" << brd.w_kings << ", b: " << brd.b_items << "/" << brd.b_kings << "\n";
        const char* cols = "    a   b   c   d   e   f   g   h";
        const char* line = "  +---+---+---+---+---+---+---+---+";
        os << line << "\n";
        for (size_t row = 0; row < 8; row++) {
            os << 8 - row << " |";
            for (size_t col = 0; col < 8; col++) {
                auto s = brd.state[row][col];
                if (is_valid_pos(row, col) && is_valid(s)) {
                    os << " " << (char)s << " |";
                } else if (!is_valid_pos(row, col) && !is_occupied(s)) {
                    os << "   |";
                } else {
                    os << "###|";
                }
            }
            os << "\n" << line << "\n";
        }
        os << cols << "\n\n";
        return os;
    }
};


inline const board_2d_t initial_board_2d = {
    12, 0, 12, 0,
    {
        {_, x, _, x, _, x, _, x},
        {x, _, x, _, x, _, x, _},
        {_, x, _, x, _, x, _, x},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {o, _, o, _, o, _, o, _},
        {_, o, _, o, _, o, _, o},
        {o, _, o, _, o, _, o, _}
    }
};

inline const board_2d_t empty_board_2d = {
    0, 0, 0, 0,
    {
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _},
        {_, _, _, _, _, _, _, _}
    }
};


board_2d_t from_1d_brd(const board_state_t& b)
{
    board_2d_t r = empty_board_2d;

    for(brd_index_t i = 0; i; ++i) {
        brd_item_t item{i};
        brd_2d_vector_t v{i};
        int row = 7 - v.y;
        int col = v.x;

        if (b.sides[0].items.exist(item)) {
            r.w_items++;
            if (b.sides[0].kings.exist(item)) {
                r.w_kings++;
                r.state[row][col] = board_cell_state_enum::G;
            } else {
                r.state[row][col] = board_cell_state_enum::o;
            }
        }

        if (b.sides[1].items.exist(item)) {
            r.b_items++;
            if (b.sides[1].kings.exist(item)) {
                r.b_kings++;
                r.state[row][col] = board_cell_state_enum::M;
            } else {
                r.state[row][col] = board_cell_state_enum::x;
            }
        }
    }

    return r;
}

board_state_t to_1d_brd(const board_2d_t& b)
{
    board_state_t r;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            brd_item_t item{brd_2d_vector_t{col, 7 - row}};
            if (is_white(b.state[row][col]))
            {
                r.sides[0].items += item;
                if (is_king(b.state[row][col])) {
                    r.sides[0].kings += item;
                }
            }
            if (is_black(b.state[row][col]))
            {
                r.sides[1].items += item;
                if (is_king(b.state[row][col])) {
                    r.sides[1].kings += item;
                }
            }
        }
    }

    return r;
}

