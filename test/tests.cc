#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>

#include <boost/functional/hash.hpp>

#include "draughts_2d.h"
#include "cases.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"




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
                    if (is_white_king_row(row)) {
                        REQUIRE(is_king(s));
                    }
                }
                if (is_black(s)) {
                    b_items++;
                    if (is_king(s)) {
                        b_kings++;
                    }
                    if (is_black_king_row(row)) {
                        REQUIRE(is_king(s));
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


TEST_CASE("item_moves")
{
    for (const auto& p : item_moves_data) {
        test_generation(p.first, p.second);
    }
}

TEST_CASE("item_capture")
{

}

TEST_CASE("item_multiple_capture")
{

}

TEST_CASE("king_moves")
{

}

TEST_CASE("king_capture")
{

}

TEST_CASE("king_multiple_capture")
{

}

