#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>

#include <boost/functional/hash.hpp>

#include "draughts.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"




enum board_cell_state_enum : char
{
    _ = ' ',
    o = 'o',
    G = 'G',
    x = 'x',
    M = 'M'
};


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

bool is_valid_pos(size_t row, size_t col)
{
    return row < 8 && col < 8 && valid_positions[row][col];
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

const board_2d_t empty_board_2d = {
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

// validate 2d board state
// - only black squares occupied
// - count existing elements
void is_valid(const board_2d_t& brd)
{
    size_t w_items = 0;
    size_t w_kings = 0;
    size_t b_items = 0;
    size_t b_kings = 0;

    for (size_t row = 0; row < 8; row++) {
        for (size_t col = 0; col < 8; col++) {
            auto s = brd.state[row][col];
            REQUIRE(is_valid(s));
            if (is_valid_pos(row, col)) {
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
                REQUIRE(!is_occupied(s));
            }
        }
    }

    REQUIRE(w_items == brd.w_items);
    REQUIRE(w_kings == brd.w_kings);
    REQUIRE(b_items == brd.b_items);
    REQUIRE(b_kings == brd.b_kings);
    REQUIRE(w_items <= 12);
    REQUIRE(w_kings <= w_items);
    REQUIRE(b_items <= 12);
    REQUIRE(b_kings <= b_items);
}


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


TEST_CASE("valid_board")
{
    board_2d_t boards[] = {
        empty_board_2d,
        initial_board_2d,
        from_1d_brd({board_side_t{0, 0}, board_side_t{0, 0}}),
        from_1d_brd(initial_board)
    };

    for (const auto& b : boards) {
        INFO("board:\n" << b);
        is_valid(b);
    }

    REQUIRE(empty_board_2d == from_1d_brd({board_side_t{0, 0}, board_side_t{0, 0}}));
    REQUIRE(initial_board_2d == from_1d_brd(initial_board));
}

TEST_CASE("2d_1d_conversion")
{
    auto b1 = to_1d_brd(initial_board_2d);
    auto b2 = from_1d_brd(b1);
    INFO("board:\n" << b2);
    is_valid(b2);
    REQUIRE(b2 == initial_board_2d);

    for (size_t i = 0; i < 32; i++) {
        brd_map_t m = 1 << i;

        board_state_t b1 = {board_side_t{0, m}, board_side_t{0, 0}};
        board_2d_t b2 = from_1d_brd(b1);
        is_valid(b2);
        REQUIRE(b1 == to_1d_brd(b2));
        
        b1 = {board_side_t{0, 0}, board_side_t{0, m}};
        b2 = from_1d_brd(b1);
        is_valid(b2);
        REQUIRE(b1 == to_1d_brd(b2));
        
        b1 = {board_side_t{m, m}, board_side_t{0, 0}};
        b2 = from_1d_brd(b1);
        is_valid(b2);
        REQUIRE(b1 == to_1d_brd(b2));

        b1 = {board_side_t{0, 0}, board_side_t{m, m}};
        b2 = from_1d_brd(b1);
        is_valid(b2);
        REQUIRE(b1 == to_1d_brd(b2));

        for (size_t j = 0; j < 32; j++) {
            if (j == i) {
                continue;
            }
            brd_map_t n = 1 << j;

            board_state_t b1 = {board_side_t{0, m}, board_side_t{0, n}};
            board_2d_t b2 = from_1d_brd(b1);
            is_valid(b2);
            REQUIRE(b1 == to_1d_brd(b2));
            
            b1 = {board_side_t{m, m}, board_side_t{n, n}};
            b2 = from_1d_brd(b1);
            is_valid(b2);
            REQUIRE(b1 == to_1d_brd(b2));
        }
    }
}



TEST_CASE("rotation")
{

}

void test_generation(const board_2d_t& initial, const std::vector<board_2d_t>& expected)
{
    is_valid(initial);
    std::unordered_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>> ex_set;
    for (const auto& b : expected) {
        is_valid(b);
        ex_set.insert(std::pair<uint64_t, uint64_t>(to_1d_brd(b)));
    }
    REQUIRE(ex_set.size() == expected.size());

    board_states_generator g;

    board_state_t s = to_1d_brd(initial);

    const auto& v = g.gen_next_states(s);

    REQUIRE(v.size() == expected.size());

    for (const auto& b : v) {
        REQUIRE(ex_set.count(std::pair<uint64_t, uint64_t>(b)) == 1);
    }
}

std::vector<std::pair<board_2d_t, std::vector<board_2d_t>>> item_moves_data = {
    {
        {0, 0, 0, 0, {
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _}
        }},
        {}
    },
    {
        {1, 0, 0, 0, {
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {o, _, _, _, _, _, _, _}
        }},
        {
            {1, 0, 0, 0, {
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, o, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _}
            }}
        }
    },
    {
        {1, 0, 0, 0, {
            {_, _, _, _, _, _, _, _},
            {o, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _},
            {_, _, _, _, _, _, _, _}
        }},
        {
            {1, 1, 0, 0, {
                {_, G, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _},
                {_, _, _, _, _, _, _, _}
            }}
        }
    }
};
TEST_CASE("item_moves")
{
    for (const auto& p : item_moves_data) {
        test_generation(p.first, p.second);
    }
}

TEST_CASE("item_capture")
{

}

TEST_CASE("king_moves")
{

}

TEST_CASE("king_capture")
{

}

