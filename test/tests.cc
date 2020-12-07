#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>
#include <iterator>

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

    INFO("board:\n" << brd);

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

void test_conversion(const board_2d_t& b2)
{
    INFO("board:\n" << b2);
    is_valid(b2);
    auto b1 = to_1d_brd(b2);
    auto b22 = from_1d_brd(b1);
    is_valid(b22);
    REQUIRE(b2 == b22);
}

board_2d_t add_item(board_2d_t b, board_cell_state_enum v, size_t row, size_t col)
{
    board_2d_t r = b;

    if (is_occupied(r.state[row][col])) {
        return r;
    }

    if (is_white(v) && is_white_king_row(row)) {
        v = board_cell_state_enum::G;
    }
    if (is_black(v) && is_black_king_row(row)) {
        v = board_cell_state_enum::M;
    }

    r.state[row][col] = v;

    if (is_white(v)) {
        r.w_items++;
        if (is_king(v)) {
            r.w_kings++;
        }   
    } else if (is_black(v)) {
        r.b_items++;
        if (is_king(v)) {
            r.b_kings++;
        }
    }

    return r;
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

void test_rotation(const board_2d_t& b2)
{
    INFO("board:\n" << b2);
    is_valid(b2);
    auto b1 = to_1d_brd(b2);
    auto b12 = rotate(b1);
    auto b13 = rotate(b12);
    auto b22 = from_1d_brd(b13);
    is_valid(b22);
    REQUIRE(b2 == b22);
}

TEST_CASE("rotation")
{
    test_rotation(empty_board_2d);

    for (size_t row = 0; row < 8; row++) {
        for (size_t cell = 0; cell < 8; cell++) {
            if (!is_valid_pos(row, cell)) {
                continue;
            }
            for (board_cell_state_enum v : available_items) {

                auto b = add_item(empty_board_2d, v, row, cell);
                test_rotation(b);

                for (size_t row = 0; row < 8; row++) {
                    for (size_t cell = 0; cell < 8; cell++) {
                        for (board_cell_state_enum v : available_items) {
                            if (!is_valid_pos(row, cell)) {
                                continue;
                            }

                            auto b2 = add_item(b, v, row, cell);
                            test_rotation(b2);
                        }
                    }
                }
            }
        }
    }

    test_rotation(initial_board_2d);
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v)
{
    if (!v.empty()) {
        std::for_each(v.begin(), v.end(), [&out] (const T& item) {
            out << item << "\n";
        });
    }
    return out;
}


void test_generation(const board_2d_t& initial, const std::vector<board_2d_t>& expected)
{
    is_valid(initial);
    std::unordered_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>> ex_set;
    for (const auto& b : expected) {
        is_valid(b);
        auto r = ex_set.insert(std::pair<uint64_t, uint64_t>(to_1d_brd(b)));
        INFO("board:\n" << b);
        REQUIRE(r.second);
    }
    REQUIRE(ex_set.size() == expected.size());

    board_states_generator g;

    board_state_t s = to_1d_brd(initial);

    const auto& v = g.gen_next_states(s);

    std::vector<board_2d_t> r;
    for (const auto& b : v) {
        r.push_back(from_1d_brd(b));
    }

    INFO("board:\n" << initial);
    INFO("boards 1d:\n" << v);
    INFO("boards 2d:\n" << r);
    REQUIRE(v.size() == expected.size());

    for (const auto& b : v) {
        INFO("board:\n" << from_1d_brd(b));
        REQUIRE(ex_set.count(std::pair<uint64_t, uint64_t>(b)) == 1);
    }
}

void test_cases(const std::vector<std::pair<board_2d_t, std::vector<board_2d_t>>>& cases)
{
    for (const auto& p : cases) {
        test_generation(p.first, p.second);
    }
}

TEST_CASE("item_moves")
{
    test_cases(item_moves_data);
}

TEST_CASE("item_capture")
{
    test_cases(item_captures_data);
}

TEST_CASE("item_multiple_capture")
{
    test_cases(item_multiple_captures_data);
}

TEST_CASE("king_moves")
{
    test_cases(king_moves_data);
}

TEST_CASE("king_capture")
{
    test_cases(king_captures_data);
}

TEST_CASE("king_multiple_capture")
{
    test_cases(king_multiple_captures_data);
}

