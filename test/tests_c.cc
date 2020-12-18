#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <set>
#include <iterator>

#include "cases.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "draughts_2d.h"


// validate 2d board state
// - only black squares occupied
// - count existing elements
void is_valid(const board_2d_t& brd)
{
    size_t w_items = 0;
    size_t w_kings = 0;
    size_t b_items = 0;
    size_t b_kings = 0;

    INFO("is_valid() board: " << brd);

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
        from_1d_brd(board_t{0, 0, 0, 0}),
        from_1d_brd(get_initial_board())
    };

    for (const auto& b : boards) {
        INFO("valid_board: board: " << b);
        is_valid(b);
    }

    REQUIRE(empty_board_2d == from_1d_brd(board_t{0, 0, 0, 0}));
    REQUIRE(initial_board_2d == from_1d_brd(get_initial_board()));
}

void test_conversion(const board_2d_t& b2)
{
    INFO("test_conversion: board: " << b2);
    is_valid(b2);
    board_t b1 = to_1d_c_brd(b2);
    INFO("test_conversion: b1: " << b1);
    board_2d_t b22 = from_1d_brd(b1);
    INFO("test_conversion: board: " << b22);
    is_valid(b22);
    REQUIRE(b2 == b22);
}

TEST_CASE("2d_1d_conversion")
{
    test_conversion(empty_board_2d);

    for (size_t row = 0; row < 8; row++) {
        for (size_t cell = 0; cell < 8; cell++) {
            if (!is_valid_pos(row, cell)) {
                continue;
            }
            for (board_cell_state_enum v : available_items) {

                auto b = add_item(empty_board_2d, v, row, cell);
                test_conversion(b);

                for (size_t row = 0; row < 8; row++) {
                    for (size_t cell = 0; cell < 8; cell++) {
                        for (board_cell_state_enum v : available_items) {
                            if (!is_valid_pos(row, cell)) {
                                continue;
                            }

                            auto b2 = add_item(b, v, row, cell);
                            test_conversion(b2);
                        }
                    }
                }
            }
        }
    }

    test_conversion(initial_board_2d);
}


TEST_CASE("validation")
{
    for (const auto& test_data : validation_test_data) {
        auto board = test_data.first;
        for (const auto& item : test_data.second) {
            std::set<std::pair<unsigned int, unsigned int>> valid_moves;
            for (const auto& dst : item.second) {
                valid_moves.insert({item.first, dst});
            }
            REQUIRE_EQ(valid_moves.size(), item.second.size());

            INFO("validation: board: " << board);
            is_valid(board);
            board_t brd = to_1d_c_brd(board);

            for (unsigned int from = 0; from <= 32; from++) {
                for (unsigned int to = 0; to <= 32; to++) {
                    int expected = valid_moves.count({from, to}) > 0 ? 1 : 0;
                    INFO("validation: move: from=" << from << ", to=" << to << ", expected=" << expected);
                    REQUIRE_EQ(verify_move(brd, from, to), expected);
                }
            }
        }
    }
}




