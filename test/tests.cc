#include <string>
#include <iostream>
#include <vector>
#include <array>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"


enum board_cell_state_enum
{
    _ = 0,
    o = 1,
    G = 2,
    x = 3,
    M = 4
};

const char printable_items[5] = {' ', 'o', 'G', 'x', 'M'};

bool is_valid(board_cell_state_enum state)
{
    return state >= 0 && state <= 4;
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
        for (size_t y = 0; y < 8; y++) {
            for (size_t x = 0; x < 8; x++) {
                if (lhs.state[y][x] != rhs.state[y][x]) { 
                    return false;
                }
            }
        }
        return true;
    }
};

const board_state_2d_t valid_positions = {
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _},
    {_, o, _, o, _, o, _, o},
    {o, _, o, _, o, _, o, _}
};

bool is_valid_pos(size_t x, size_t y)
{
    return x < 8 && y < 8 && valid_positions[y][x];
}

const board_2d_t initial_board_2d = {
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

void print(const board_2d_t& brd)
{
    const char* cols = "    a   b   c   d   e   f   g   h";
    const char* line = "  +---+---+---+---+---+---+---+---+";
    printf("%s\n", line);
    for (size_t y = 0; y < 8; y++) {
        printf("%lu |", 8 - y);
        for (size_t x = 0; x < 8; x++) {
            auto s = brd.state[y][x];
            if (is_valid_pos(x, y) && is_valid(s)) {
                printf(" %c |", printable_items[s]);
            } else if (!is_valid_pos(x, y) && !is_occupied(s)) {
                printf("   |");
            } else {
                printf("###|");
            }
        }
        printf("\n%s\n", line);
    }
    printf("%s\n\n", cols);
}

// validate 2d board state
// - only black squares occupied
// - count existing elements
bool is_valid(const board_2d_t& brd)
{
    size_t w_items = 0;
    size_t w_kings = 0;
    size_t b_items = 0;
    size_t b_kings = 0;

    for (size_t y = 0; y < 8; y++) {
        for (size_t x = 0; x < 8; x++) {
            auto s = brd.state[y][x];
            if (!is_valid(s)) {
                return false;
            }
            if (is_valid_pos(x, y)) {
                if (is_white(s)) {
                    w_items++;
                    if (is_king(s)) {
                        w_kings++;
                    }
                }
                if (is_black(s)) {
                    b_items++;
                    if (is_king(s)) {
                        b_kings++;
                    }
                }
            } else {
                if (is_occupied(s)) {
                    return false;
                }
            }
        }
    }

    return w_items == brd.w_items &&
           w_kings == brd.w_kings &&
           b_items == brd.b_items &&
           b_kings == brd.b_kings;
}





TEST_CASE("valid_initial_board")
{
    REQUIRE(is_valid(initial_board_2d));
}


