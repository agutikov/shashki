
#include <cstdio>
#include <array>
#include <utility>
#include <vector>
#include <algorithm>
#include <tuple>
#include <stack>
#include <chrono>
#include <map>
#include <iostream>
#include <iomanip>
#include <initializer_list>
#include <random>
#include <unordered_set>
#include <type_traits>
#include <thread>
#include <future>
#include <ctime>
#include <csignal>

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/program_options.hpp>
#include <boost/functional/hash.hpp>

#include <google/dense_hash_set>
#include <google/sparse_hash_set>

#include <Judy.h>


namespace po = boost::program_options;

using namespace std::string_literals;

using namespace std::literals::chrono_literals;
using Clock = std::chrono::system_clock;


typedef std::pair<int, int> X_t;

constexpr X_t operator+(const X_t& l, const X_t& r)
{   
    return {l.first + r.first, l.second + r.second};                                    
}

constexpr X_t operator*(const X_t& l, int r)
{   
    return {l.first*r, l.second*r};                                    
}

constexpr bool is_valid(const X_t& x)
{
    return x.first >= 0 && x.first < 8 && x.second >= 0 && x.second < 8;
}

constexpr int pos2index(const X_t& x)
{
    return (8*x.second + x.first) / 2;
}

constexpr uint32_t pos2bitmap(const X_t& x)
{
    return 1 << pos2index(x);
}

constexpr uint32_t index2bitmap(int index)
{
    return 1 << index;
}


constexpr auto gen_pos_table()
{
    std::array<X_t, 32> r{X_t{0, 0}};

    X_t x{0, 0};
    for (int i = 0; i < 32; i++) {
        r[i] = x;
        x.first += 2;
        if ((x.first / 8) > 0) {
            x.second++;
            x.first %= 8;
            x.first += (x.second % 2 > 0) ? 1 : -1;
        }
    }

    return r;
}

const std::array<X_t, 32> pos_table = gen_pos_table();

constexpr auto index2pos(int i)
{
    return pos_table[i];
} 

//TODO: possible representations of item: bitmap, index, coordinates {x, y}, string e.g. "e5"
//TODO: conversions between each representations

//TODO: same for one-step moves: bitmap-bitmap, index-index, x,y-x,y, "e5-f6"
//TODO: complete move with captures - more complicated, use board states instead 


//TODO: magic constants, related to type size


template <size_t L>
class moves_iter
{
public:
    constexpr moves_iter(const X_t& x, const std::array<X_t, L>& m) :
        x(x), moves(m)
    {
        operator++();
    }

    void operator++()
    {
        do {
            v = x + moves[i++];
        } while (!is_valid(v) && i < moves.size());
    }

    explicit operator bool() const
    {
        return is_valid(v) && i <= moves.size();
    }

    X_t operator*() const
    {
        return v;
    }

    const X_t* operator->() const
    {
        return &v;
    }
private:
    X_t x;
    std::array<X_t, L> moves;
    size_t i = 0;
    X_t v;
};

template <size_t L>
constexpr auto product(const std::array<int, L>& x)
{
    std::array<X_t, L*L> r{X_t{0, 0}};
    size_t i = 0;
    for (int x1 : x) {
        for (int x2 : x) {
            r[i++] = {x1, x2};
        }
    }
    return r;
}

const std::array<X_t, 2> fwd_directions{X_t{-1,  1}, X_t{1,  1}};
const std::array<X_t, 4> all_directions = product(std::array<int, 2>{-1, 1});

const std::array<X_t, 4> short_captures_dst = product(std::array<int, 2>{-2, 2});
const std::array<X_t, 4> short_captures = product(std::array<int, 2>{-1, 1});

template<size_t L>
constexpr auto gen_moves(const std::array<X_t, L>& moves)
{
    std::array<std::array<uint32_t, L>, 32> r{0};

    for (int index = 0; index < 32; index++) {
        int i = 0;
        r[index].fill(0);
        for (moves_iter<L> mov(index2pos(index), moves); mov; ++mov) {//TODO: remove moves_iter
            r[index][i++] = pos2bitmap(*mov);
        }
    }

    return r;
}

template <size_t L1, size_t L2>
constexpr auto product(const std::array<X_t, L1>& x, const std::array<int, L2>& m)
{
    std::array<X_t, L1*L2> r{X_t{0, 0}};
    size_t i = 0;
    for (X_t x_i : x) {
        for (int m_j : m) {
            r[i++] = x_i * m_j;
        }
    }
    return r;
}

const std::array<X_t, 28> long_moves = product(all_directions, std::array<int, 7>{1, 2, 3, 4, 5, 6, 7});
const std::array<X_t, 24> long_captures = product(all_directions, std::array<int, 6>{1, 2, 3, 4, 5, 6});
const std::array<X_t, 24> long_jumps = product(all_directions, std::array<int, 6>{2, 3, 4, 5, 6, 7});

template<size_t L>
constexpr auto moves_mask(const std::array<X_t, L>& moves)
{
    std::array<uint32_t, 32> r{0};
    r.fill(0);

    for (int index = 0; index < 32; index++) {
        for (moves_iter<L> mov(index2pos(index), moves); mov; ++mov) {
            r[index] |= pos2bitmap(*mov);
        }
    }

    return r;
}

template<size_t L>
constexpr auto gen_captures(const std::array<X_t, L>& moves, const std::array<X_t, L>& captures)
{
    std::array<std::array<std::pair<uint32_t, int>, L>, 32> r{{std::pair<uint32_t, int>{0,0}}};

    for (int index = 0; index < 32; index++) {
        auto x = index2pos(index);
        auto& out_a = r[index];
        out_a.fill({0,0});

        std::array<std::pair<X_t, X_t>, L> zip;
        std::transform(moves.begin(), moves.end(), captures.begin(), zip.begin(),
            [] (auto aa, auto bb) {
                return std::pair<X_t, X_t>(aa, bb);
            }
        );

        int i = 0;
        for (auto p : zip) {
            X_t dst = x + p.first;
            X_t cap = x + p.second;
            if (is_valid(dst)) {
                out_a[i++] = std::pair<uint32_t, int>(pos2bitmap(cap), pos2index(dst));
            }
        }
    }

    return r;
}

constexpr auto gen_king_moves()
{
    std::array<std::array<std::array<uint32_t, 7>, 4>, 32> r{{{0}}};
    const auto m7 = std::array<int, 7>{1, 2, 3, 4, 5, 6, 7};

    const std::array<std::array<X_t, 7>, 4> moves = {
        product(std::array<X_t, 1>{X_t{-1, -1}}, m7),
        product(std::array<X_t, 1>{X_t{ 1, -1}}, m7),
        product(std::array<X_t, 1>{X_t{-1,  1}}, m7),
        product(std::array<X_t, 1>{X_t{ 1,  1}}, m7),
    };

    for (int index = 0; index < 32; index++) {
        auto x = index2pos(index);
        auto& out_a = r[index];

        for (size_t dir = 0; dir < 4; dir++){
            auto& out_dir = out_a[dir];
            out_dir.fill(0);

            const auto& dir_moves = moves[dir];
            int i = 0;
            for (auto move : dir_moves) {
                auto dst = x + move;
                if (is_valid(dst)) {
                    out_dir[i++] = pos2bitmap(dst);
                }
            }
        }
    }

    return r;
}

constexpr auto gen_king_captures()
{
    std::array<std::array<std::array<std::pair<uint32_t, std::array<int, 6>>, 6>, 4>, 32>
    r{{{std::pair<uint32_t, std::array<int, 6>>{0, {-1}}}}};

    const auto c6 = std::array<int, 6>{1, 2, 3, 4, 5, 6};
    const auto m6 = std::array<int, 6>{2, 3, 4, 5, 6, 7};

    const std::array<std::array<X_t, 6>, 4> captures = {
        product(std::array<X_t, 1>{X_t{-1, -1}}, c6),
        product(std::array<X_t, 1>{X_t{ 1, -1}}, c6),
        product(std::array<X_t, 1>{X_t{-1,  1}}, c6),
        product(std::array<X_t, 1>{X_t{ 1,  1}}, c6),
    };
    const std::array<std::array<X_t, 6>, 4> minimal_moves = {
        product(std::array<X_t, 1>{X_t{-1, -1}}, m6),
        product(std::array<X_t, 1>{X_t{ 1, -1}}, m6),
        product(std::array<X_t, 1>{X_t{-1,  1}}, m6),
        product(std::array<X_t, 1>{X_t{ 1,  1}}, m6),
    };

    std::array<std::array<std::pair<X_t, X_t>, 6>, 4> zip;
    for (int i = 0; i < 4; i++) {
        std::transform(minimal_moves[i].begin(), minimal_moves[i].end(), captures[i].begin(), zip[i].begin(),
            [] (auto aa, auto bb) constexpr {
                return std::pair<X_t, X_t>(aa, bb);
            }
        );
    }

    // for every item position
    for (int index = 0; index < 32; index++) {
        auto x = index2pos(index);
        auto& out_a = r[index];

        // for every directon from this position
        for (size_t dir = 0; dir < 4; dir++){
            auto& out_dir = out_a[dir];
            out_dir.fill(std::pair<uint32_t, std::array<int, 6>>{0, {-1}});

            const auto& dir_zip = zip[dir];

            // fill possible captures for this direction
            int c_i = 0;
            for (int c_index = 0; c_index < dir_zip.size(); c_index++) {
                auto c = dir_zip[c_index];
                auto capture = x + c.second;
                auto min_dst = x + c.first;
                if (is_valid(min_dst)) {
                    out_dir[c_i] = std::pair<uint32_t, std::array<int, 6>>{pos2bitmap(capture), {-1}};
                    out_dir[c_i].second.fill(-1);

                    // and fill possible jump destinations for this capture
                    int j_i = 0;
                    for (int j_index = c_index; j_index < dir_zip.size(); j_index++) {
                        auto j = dir_zip[j_index];
                        auto dst = x + j.first;
                        if (is_valid(dst)) {
                            out_dir[c_i].second[j_i++] = pos2index(dst);
                        }
                    }

                    c_i++;
                }
            }
        }
    }

    return r;
}

//TODO: Optimizations:
// - binary serach of moves/captures: whole bitmap -> half(forward+backward or cross) -> 4 directions
// - 32*4 different functions, maybe generated with templates
// - utilize the diagonal move symmetry, e.g. detect possible moves for multiple items same time

//TODO: Generalisation: use same function for items and kings but different control structure

const struct tables_t
{
    //TODO: RENAME

    std::array<std::array<uint32_t, 2>, 32> fwd_moves = gen_moves<2>(fwd_directions);
    std::array<uint32_t, 32> fwd_move_masks = moves_mask<2>(fwd_directions);

    // first is capture, second is move destination index
    std::array<std::array<std::pair<uint32_t, int>, 4>, 32> captures = gen_captures<4>(short_captures_dst, short_captures);
    std::array<uint32_t, 32> capture_move_masks = moves_mask<4>(short_captures_dst);
    std::array<uint32_t, 32> capture_masks = moves_mask<4>(short_captures);

    std::array<std::array<std::array<uint32_t, 7>, 4>, 32> king_moves = gen_king_moves();
    std::array<uint32_t, 32> king_move_masks = moves_mask<28>(long_moves);

    // item index -> 4 directions -> list of pairs (capture mask, list of possible jump destination indexes)
    std::array<std::array<std::array<std::pair<uint32_t, std::array<int, 6>>, 6>, 4>, 32>
    king_captures = gen_king_captures();
    std::array<uint32_t, 32> king_capture_move_masks = moves_mask<24>(long_jumps);
    std::array<uint32_t, 32> king_capture_masks = moves_mask<24>(long_captures);

} tables;


struct board_side_t
{
    uint32_t kings = 0;
    uint32_t items = 0;
    
    explicit operator uint64_t() const
    {
        return uint64_t(kings) << 32 | items;
    }
};

struct board_state_t
{
    //TODO: ctor from initializer list

    std::array<board_side_t, 2> sides;

    explicit operator std::pair<uint64_t, uint64_t>() const
    {
        //return {uint64_t(sides[0]), uint64_t(sides[1])};
        return {
            (uint64_t(sides[0].kings) << 32) | sides[1].kings,
            (uint64_t(sides[0].items) << 32) | sides[1].items
        };
    }
};


const int format_table[8][8] = {
    {-1, 28, -1, 29, -1, 30, -1, 31},
    {24, -1, 25, -1, 26, -1, 27, -1},
    {-1, 20, -1, 21, -1, 22, -1, 23},
    {16, -1, 17, -1, 18, -1, 19, -1},
    {-1, 12, -1, 13, -1, 14, -1, 15},
    { 8, -1,  9, -1, 10, -1, 11, -1},
    {-1,  4, -1,  5, -1,  6, -1,  7},
    { 0, -1,  1, -1,  2, -1,  3, -1},
};

void print(const board_state_t& s)
{
    const char* cols = "     a     b     c     d     e     f     g     h     ";
    const char* line = "  -------------------------------------------------  ";
    int row = 8;
    printf("%s\n%s\n", cols, line);
    for (const auto& f : format_table) {
        printf("%d |", row);
        for (int i : f) {
            if (i == -1) {
                printf("     |");
            } else {
                uint32_t mask = index2bitmap(i);
                const char* c = s.sides[0].kings & mask ? " (O) " :
                         s.sides[0].items & mask ? "  o  " :
                         s.sides[1].kings & mask ? " }X{ " :
                         s.sides[1].items & mask ? "  x  " : "     ";
                printf("%s|", c);
            }
        }
        printf(" %d\n%s\n", row, line);
        row--;
    }
    printf("%s\n\n", cols);
}

// https://stackoverflow.com/questions/746171/efficient-algorithm-for-bit-reversal-from-msb-lsb-to-lsb-msb-in-c
const uint8_t bit_reverse_table_256[] = 
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

uint32_t reverse(uint32_t v)
{
    return (bit_reverse_table_256[v & 0xff] << 24) | 
           (bit_reverse_table_256[(v >> 8) & 0xff] << 16) | 
           (bit_reverse_table_256[(v >> 16) & 0xff] << 8) |
           (bit_reverse_table_256[(v >> 24) & 0xff]);
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


board_state_t do_move(board_state_t state, uint32_t src, uint32_t dst)
{
    state.sides[0].items &= ~src;
    state.sides[0].items |= dst;

    if (state.sides[0].kings & src) {
        state.sides[0].kings &= ~src;
        state.sides[0].kings |= dst;
    }

    // handle King row
    if (dst & 0xF0000000) {
        state.sides[0].kings |= dst;
    }

    return state;
}

board_state_t do_capture(board_state_t state, uint32_t capture)
{
    state.sides[1].items &= ~capture;
    if (state.sides[1].kings & capture) {
        state.sides[1].kings &= ~capture;
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

    size_t gen_next_states(const board_state_t& brd)
    {
        cur_state = brd;
        occupied = cur_state.sides[0].items | cur_state.sides[1].items;

        size_t count = gen_states();
        return count;
    }

private:

    // move active item between recursive calls (change state) and collect captured enemies
    // when capture sequence completed - remove captured enemies before state saved
    size_t next_item_captures(const board_state_t& state, int item_index, uint32_t captured)
    {
        // Rules:
        // - enemy items have to be removed after capture chain finished completely
        // - every enemy item can't be captured twice
        uint32_t moves_mask = tables.capture_move_masks[item_index];
        uint32_t available_dst = (moves_mask & (state.sides[0].items | state.sides[1].items)) ^ moves_mask;
        uint32_t may_be_captured = tables.capture_masks[item_index] & state.sides[1].items;

        // not allow capture same items multiple times
        may_be_captured &= ~captured;

        size_t saved_states = 0;
        if (may_be_captured != 0 && available_dst != 0) {
            for (auto [capture_mask, dst_index] : tables.captures[item_index]) {
                uint32_t dst_mask = index2bitmap(dst_index);
                if (dst_mask == 0) {
                    break;
                }
                // branching possible capture moves, follow moves
                if ((capture_mask & may_be_captured) && (dst_mask & available_dst)) {
                    // do move
                    board_state_t next_state = do_move(state, index2bitmap(item_index), dst_mask);

                    // try continue capturing
                    saved_states += next_item_captures(next_state, dst_index, captured | capture_mask);
                }
            }
        }

        // nothing more can be captured
        if (saved_states == 0) {
            // capture sequence completed
            if (captured != 0) {
                // do capture
                board_state_t next_state = do_capture(state, captured);

                // save new state if it is final
                _states.push_back(next_state);
                return 1;
            } else {
                return 0;
            }
        }

        return saved_states;
    }

    size_t next_item_moves(int item_index)
    {
        uint32_t moves_mask = tables.fwd_move_masks[item_index];
        uint32_t available_dst = (moves_mask & occupied) ^ moves_mask;

        if (available_dst == 0) {
            return 0;
        }

        size_t saved_states = 0;
        for (uint32_t move : tables.fwd_moves[item_index]) {
            if (move == 0) {
                break;
            }
            if (move & available_dst) {
                // do move - get new board state
                board_state_t next_state = do_move(cur_state, index2bitmap(item_index), move);
                // save new state
                _states.push_back(next_state);
                saved_states++;
            }
        }
        return saved_states;
    }

    //TODO: Generalize items and kings tables and next_* functions, parametrize function template with const table

    size_t next_king_captures(const board_state_t& state, int item_index, uint32_t captured)
    {
        uint32_t cur_occupied = state.sides[0].items | state.sides[1].items;
        uint32_t moves_mask = tables.king_capture_move_masks[item_index];
        uint32_t available_dst = (moves_mask & cur_occupied) ^ moves_mask;
        uint32_t may_be_captured = tables.king_capture_masks[item_index] & state.sides[1].items;

        // not allow capture same items multiple times
        may_be_captured &= ~captured;

        size_t saved_states = 0;
        if (may_be_captured != 0 && available_dst != 0) {
            // iter directions
            for (const auto& dir_captures : tables.king_captures[item_index]) {
                // iter captures in direction
                for (const auto& dir_capture : dir_captures) {
                    uint32_t capture_mask = dir_capture.first;

                    if (capture_mask == 0) {
                        break;
                    }
                    // cant't jump over allies
                    if (capture_mask & state.sides[0].items) {
                        break;
                    }

                    if (capture_mask & may_be_captured) {
                        // iter over possible jump destinations
                        for (int dst_index : dir_capture.second) {
                            if (dst_index < 0) {
                                break;
                            }
                            uint32_t dst_mask = index2bitmap(dst_index);

                            // cant't jump over allies or capture/jump over multiple enemy items
                            if (dst_mask & cur_occupied) {
                                break;
                            }

                            if (dst_mask & available_dst) {
                                // do move
                                board_state_t next_state = do_move(state, index2bitmap(item_index), dst_mask);

                                // try continue capturing
                                saved_states += next_item_captures(next_state, dst_index, captured | capture_mask);
                            }
                        }
                    }
                }
            }
        }

        // nothing more can be captured
        if (saved_states == 0) {
            // capture sequence completed
            if (captured != 0) {
                // do capture
                board_state_t next_state = do_capture(state, captured);

                // save new state if it is final
                _states.push_back(next_state);
                return 1;
            } else {
                return 0;
            }
        }

        return saved_states;
    }

    size_t next_king_moves(int item_index)
    {
        uint32_t moves_mask = tables.king_move_masks[item_index];
        uint32_t available_dst = (moves_mask & occupied) ^ moves_mask;

        if (available_dst == 0) {
            return 0;
        }

        size_t saved_states = 0;
        for (const auto& move_dir : tables.king_moves[item_index]) {
            for (uint32_t move : move_dir) {
                if (move == 0) {
                    break;
                }
                if (move & available_dst) {
                    // do move - get new board state
                    board_state_t next_state = do_move(cur_state, index2bitmap(item_index), move);
                    // save new state
                    _states.push_back(next_state);
                    saved_states++;
                } else {
                    // capture handled in next_king_captures()
                    // can't jump while move without capture
                    break;
                }
            }
        }
        return saved_states;
    }

    size_t gen_states()
    {
        size_t saved_states = 0;
        const board_side_t& player = cur_state.sides[0];

        for (int item_index = 0; item_index < 32; item_index++) {
            uint32_t item = index2bitmap(item_index);
            if (item & player.kings) {
                saved_states += next_king_captures(cur_state, item_index, 0);
            } else if (item & player.items) {
                saved_states += next_item_captures(cur_state, item_index, 0);
            }
        }

        // As capture is mandatory and can't be skipped
        // So first - try capture, and if no available captures - then try move
        if (saved_states) {
            return saved_states;
        }

        for (int item_index = 0; item_index < 32; item_index++) {
            uint32_t item = index2bitmap(item_index);
            if (item & player.kings) {
                saved_states += next_king_moves(item_index);
            } else if (item & player.items) {
                saved_states += next_item_moves(item_index);
            }
        }

        return saved_states;
    }

    uint32_t occupied;
    board_state_t cur_state;

    std::vector<board_state_t>& _states;
};

// experimantal 
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

    std::vector<board_state_t> states;

private:
    _board_states_generator g;
};


struct judy_128_set
{
    std::pair<void*, bool> insert(const std::pair<uint64_t, uint64_t>& v)
    {
        // insert or get
        void** pv = JudyLIns(&array, v.first, nullptr);

        if (pv == PJERR) {
            return {nullptr, false};
        }

        int inserted = Judy1Set(pv, v.second, nullptr);

        _size += inserted;
        
        return {nullptr, inserted == 1};
    }

    size_t size() const
    {
        return _size;
    }

    ~judy_128_set()
    {
        uint64_t index = 0;
        void** pv = JudyLFirst(array, &index, nullptr);
        while (pv != nullptr) {
            Judy1FreeArray(pv, nullptr);
            pv = JudyLNext(array, &index, nullptr);
        }
        JudyLFreeArray(&array, nullptr);
    }

    void dump()
    {
        printf("size=%lu\n", _size);
        uint64_t l1 = 0;
        void** pv = JudyLFirst(array, &l1, nullptr);
        while (pv != nullptr) {
            uint64_t l2 = 0;
            int r = Judy1First(*pv, &l2, nullptr);
            for (;;) {
                if (r) {
                    printf("0x%08lX, 0x%08lX\n", l1, l2);
                } else {
                    break;
                }
                r = Judy1Next(*pv, &l2, nullptr);
            }
            pv = JudyLNext(array, &l1, nullptr);
        }
    }

private:
    void* array = nullptr;
    size_t _size = 0;
};


void debug_judy_128_set()
{
    judy_128_set s;

    s.insert({0x2ABCD, 0x11111});
    s.insert({0xABCD, 0x11112});
    s.insert({0xABCD1, 0x11131});
    s.insert({0x9ABCD1, 0x11411});
    s.insert({0xABCD, 0x11112});
    s.insert({0xABCD, 0x11112});
    s.insert({0xABCD, 0x11112});

    s.dump();
}



//TODO: collect stats:
// - total boards number, per depth, and sum up to depth
// - level width total max/mean/min
// - level width total histogram
// - level width max/mean/min per depth
// - level width histogram per depth
// - paths total number (!= number of boards on last level)
// - paths depth total max/mean/min
// - paths depth total histogram
// - paths depth histogram per win/lose/draw/unfinished
// - number of items, number of kings, number of items+kings: 3 histograms per depth
// - cache stats: hits, ...

template<typename T>
float total_seconds(T d)
{
    using fsecs = std::chrono::duration<float, std::chrono::seconds::period>;
    return std::chrono::duration_cast<fsecs>(d).count();
}

struct stats
{
    void consume_level_width(size_t w, size_t depth)
    {
        _total_boards += w;

        if (w >= level_width_hist.size()) {
            level_width_hist.resize(w + 1, 0);
        }
        level_width_hist[w]++;
        
        if (w == 0) {
            if (depth % 2 == 0) {
                b_wins++;
            } else {
                w_wins++;
            }
        }
    }

    void cache_hit()
    {
        cache_hits++;
    }

    void depth_limit()
    {
        depth_limits++;
    }

    void print(Clock::time_point started, size_t j)
    {
        float elapsed_s = total_seconds(Clock::now() - started);
        printf("\nelapsed: %fs\n", elapsed_s);

        printf("total boards: %lu\n", _total_boards);

        printf("rate: %.2f Mboards/s\n", _total_boards / elapsed_s / 1000000);
        printf("rate/thread: %.2f Mboards/s\n", _total_boards / elapsed_s / 1000000 / j);

        printf("\nlevel width (number of possible moves) histogram:\n");
        for (int w = 0; w < level_width_hist.size(); w++) {
            printf("%2d: %lu\n", w, level_width_hist[w]);
        }
        printf("\n");

        printf("W wins: %lu; B wins: %lu; depth limits: %lu; cache hits: %lu\n", w_wins, b_wins, depth_limits, cache_hits);
    }

    size_t total_boards() const
    {
        return _total_boards;
    }

    stats& operator+=(stats& other)
    {
        _total_boards += other._total_boards;
        w_wins += other.w_wins;
        b_wins += other.b_wins;
        depth_limits += other.depth_limits;
        cache_hits += other.cache_hits;

        if (other.level_width_hist.size() > level_width_hist.size()) {
            level_width_hist.resize(other.level_width_hist.size(), 0);
        }
        if (other.level_width_hist.size() < level_width_hist.size()) {
            other.level_width_hist.resize(level_width_hist.size(), 0);
        }

        std::transform(
            level_width_hist.begin(),
            level_width_hist.end(),
            other.level_width_hist.begin(),
            level_width_hist.begin(),
            std::plus<>{}
        );

        return *this;
    }

private:
    size_t _total_boards = 0;
    std::vector<size_t> level_width_hist;
    size_t w_wins = 0;
    size_t b_wins = 0;
    size_t depth_limits = 0;
    size_t cache_hits = 0;
};

void print_tp(const Clock::time_point& tp)
{
    auto t = Clock::to_time_t(tp);
    std::cout << std::put_time(std::localtime(&t), "%F %T") << std::endl;
}


using dense_cache = google::dense_hash_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>>;
using judy_cache = judy_128_set;
using std_cache = std::unordered_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>>;

bool g_running = true;

template<class Cache>
struct DFS
{
    DFS(size_t max_depth, 
        bool verbose = true,
        Clock::duration timeout = 5min,
        size_t max_width = 0,
        bool randomize = false,
        bool cache = false,
        bool print_win_path = false,
        bool print_cache_hit_board = false)
    :
        max_depth(max_depth),
        randomize(randomize),
        max_width(max_width),
        verbose(verbose),
        timeout(timeout),
        stack(max_depth),
        enable_cache(cache),
        print_win_path(print_win_path),
        print_cache_hit_board(print_cache_hit_board)
    {
        std::mt19937 rng{std::random_device{}()};
        for (size_t i = 0; i < MAX_LEVEL_WIDTH; i++) {
            std::vector<size_t> v(i);
            std::iota(v.begin(), v.end(), 0);
            std::shuffle(v.begin(), v.end(), rng);
            random_indexes.emplace_back(std::move(v));
        }

        path.reserve(max_depth);

        if constexpr (std::is_same<Cache, dense_cache>::value) {
            boards_cache.set_empty_key({0, 0});
        }
    }

    void do_search(const board_state_t& brd)
    {
        std::cout << std::boolalpha
                  << "DFS, max_depth=" << max_depth
                  << ", timeout=" << total_seconds(timeout) << "s"
                  << ", max_width=" << max_width
                  << ", randomize=" << randomize
                  << ", cache=" << enable_cache 
                  << ", print_cache_hits=" << print_cache_hit_board
                  << ", print_wins=" << print_win_path
                  << std::endl;
        printf("\n  Initial board:\n");
        print(brd);

        running = true;
        started = Clock::now();
        next_status_print = started + status_print_period;
        next_total_boards = boards_count_step;
        sts = std::move(stats());
        if (print_win_path) {
            path.clear();
        }

        _search_r(stack.data(), brd, 0);

        sts.print(started, 1);
        if (enable_cache) {
            printf("\nCached: %lu boards", boards_cache.size());
            if (max_width > 0) {
                printf("\n");
            } else {
                printf(", Hits: %.2f%%\n", 100.0*(sts.total_boards() - boards_cache.size())/sts.total_boards());
            }
        }
    }

private:
    void handle_status()
    {
        if (sts.total_boards() >= next_total_boards) {
            next_total_boards += boards_count_step;
            if (Clock::now() > next_status_print) {
                next_status_print += status_print_period;
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - started);
                printf("elapsed: %lus, boards: %lu\n", elapsed.count(), sts.total_boards());
                running = Clock::now() < (started + timeout);
                if (!running) {
                    printf("Timeout.\n");
                }
                running = running && g_running;
            }
        }
    }

    void print_board(const board_state_t& brd, size_t depth, size_t branch)
    {
        printf("\n  depth: %lu; branch: %lu:\n", depth, branch);
        if (depth % 2 == 0) {
            print(rotate(brd));
        } else {
            print(brd);
        }
    }

    void print_board(const board_state_t& brd, size_t depth)
    {
        printf("\n  depth: %lu;\n", depth);
        if (depth % 2 == 0) {
            print(rotate(brd));
        } else {
            print(brd);
        }
    }

    void _handle_brd(board_states_generator* sp, const board_state_t& brd, size_t depth, size_t branch)
    {
        if (verbose) {
            print_board(brd, depth, branch);
        }

        if (enable_cache) {
            auto ins_res = boards_cache.insert(std::pair<uint64_t, uint64_t>(brd));
            if (!ins_res.second) {
                sts.cache_hit();
                if (print_cache_hit_board) {
                    printf("LOOP:\n");
                    print_board(brd, depth);
                }
                return;
            }
        }

        if (depth < max_depth) {
            _search_r(sp, rotate(brd), depth);
        } else {
            sts.depth_limit();
        }
    }

    void _search_r(board_states_generator* sp, const board_state_t& brd, size_t depth)
    {
        handle_status();
        if (!running) {
            return;
        }

        auto& v = sp->gen_next_states(brd);
        sts.consume_level_width(v.size(), depth);

        if (v.size() == 0) {
            if (print_win_path) {
                printf("%s WINS:\n", (depth % 2) ? "B" : "W");
                size_t d = 0;
                for (const auto& b : path) {
                    print_board(b, d++);
                }
                print_board(brd, depth);
                printf("%s WINS!\n\n", (depth % 2) ? "B" : "W");
            }
            return;
        }

        // go deeper
        depth++;
        sp++;
        if (print_win_path) {
            path.push_back(brd);
        }

        if (max_width == 0) {
            if (randomize) {
                // Iterate all branches in random order

                const auto& indexes = random_indexes[v.size()];
                for (size_t i = 0; i < v.size(); i++) {
                    size_t index = indexes[i];
                    _handle_brd(sp, v[index], depth, index);
                }
            } else {
                // Iterate all branches in normal order

                size_t branch = 0;
                for (const auto& next_brd : v) {
                    _handle_brd(sp, next_brd, depth, branch++);
                }
            }
        } else {
            if (randomize) {
                // Iterate limited number or branches in random order

                size_t len = std::min(max_width, v.size());
                const auto& indexes = random_indexes[v.size()];
                for (size_t i = 0; i < len; i++) {
                    size_t index = indexes[i];
                    _handle_brd(sp, v[index], depth, index);
                }
            } else {
                // Iterate limited number of branches - 1, 2 or 3

                _handle_brd(sp, v.front(), depth, 0);
                if (max_width == 3 && v.size() >= 3) {
                    size_t i = v.size() / 2;
                    _handle_brd(sp, v[i], depth, i);
                }
                if (max_width >= 2 && v.size() >= 2) {
                    _handle_brd(sp, v.back(), depth, v.size() - 1);
                }
            }
        }

        if (print_win_path) {
            path.pop_back();
        }
    }

    const size_t max_depth;
    const size_t max_width;
    const bool randomize;
    const bool verbose;
    const Clock::duration timeout;
    
    std::vector<board_states_generator> stack;

    const bool enable_cache;
    Cache boards_cache;

    const bool print_win_path;
    const bool print_cache_hit_board;
    std::vector<board_state_t> path;

    Clock::time_point started;
    Clock::time_point next_status_print;
    size_t next_total_boards;
    bool running;

    stats sts;
    
    std::vector<std::vector<size_t>> random_indexes;

    const size_t boards_count_step = 1000000;
    const Clock::duration status_print_period{2s};
};

//TODO: template with bool arg verbose

struct search_config_t
{
    size_t max_depth;
    Clock::time_point run_until;
    size_t max_width = 0;
    bool randomize = false;
    bool cache = false;
};


typedef std::tuple<stats, bool> dfs_result_t;

template<class Cache>
struct DFS_worker
{
    DFS_worker(const search_config_t& cfg)
    :
        max_depth(cfg.max_depth),
        randomize(cfg.randomize),
        max_width(cfg.max_width),
        run_until(cfg.run_until),
        stack(cfg.max_depth),
        enable_cache(cfg.cache)
    {
        std::mt19937 rng{std::random_device{}()};
        for (size_t i = 0; i < MAX_LEVEL_WIDTH; i++) {
            std::vector<size_t> v(i);
            std::iota(v.begin(), v.end(), 0);
            std::shuffle(v.begin(), v.end(), rng);
            random_indexes.emplace_back(std::move(v));
        }

        if constexpr (std::is_same<Cache, dense_cache>::value) {
            boards_cache.set_empty_key({0, 0});
        }
    }


    // return stats and completion flag
    dfs_result_t do_search(const std::vector<board_state_t>& boards, size_t depth)
    {
        running = true;
        next_total_boards = boards_count_step;

        for (const auto& brd : boards) {
            _search_r(stack.data(), brd, depth);
        }

        return {sts, running};
    }

    auto get_callable(std::vector<board_state_t>&& boards, size_t depth)
    {
        return [b{std::forward<std::vector<board_state_t>>(boards)}, this, depth] () {
            return do_search(b, depth);
        };
    }

private:
    void handle_status()
    {
        if (sts.total_boards() >= next_total_boards) {
            next_total_boards += boards_count_step;
            running = Clock::now() < run_until;
            running = running && g_running;
        }
    }

    void _handle_brd(board_states_generator* sp, const board_state_t& brd, size_t depth, size_t branch)
    {
        if (enable_cache) {
            auto ins_res = boards_cache.insert(std::pair<uint64_t, uint64_t>(brd));
            if (!ins_res.second) {
                sts.cache_hit();
                return;
            }
        }

        if (depth < max_depth) {
            _search_r(sp, rotate(brd), depth);
        } else {
            sts.depth_limit();
        }
    }

    void _search_r(board_states_generator* sp, const board_state_t& brd, size_t depth)
    {
        handle_status();
        if (!running) {
            return;
        }

        auto& v = sp->gen_next_states(brd);
        sts.consume_level_width(v.size(), depth);

        if (v.size() == 0) {
            return;
        }

        // go deeper
        depth++;
        sp++;

        if (max_width == 0) {
            if (randomize) {
                // Iterate all branches in random order

                const auto& indexes = random_indexes[v.size()];
                for (size_t i = 0; i < v.size(); i++) {
                    size_t index = indexes[i];
                    _handle_brd(sp, v[index], depth, index);
                }
            } else {
                // Iterate all branches in normal order

                size_t branch = 0;
                for (const auto& next_brd : v) {
                    _handle_brd(sp, next_brd, depth, branch++);
                }
            }
        } else {
            if (randomize) {
                // Iterate limited number or branches in random order

                size_t len = std::min(max_width, v.size());
                const auto& indexes = random_indexes[v.size()];
                for (size_t i = 0; i < len; i++) {
                    size_t index = indexes[i];
                    _handle_brd(sp, v[index], depth, index);
                }
            } else {
                // Iterate limited number of branches - 1, 2 or 3
                //TODO: do same as following split() does

                _handle_brd(sp, v.front(), depth, 0);
                if (max_width == 3 && v.size() >= 3) {
                    size_t i = v.size() / 2;
                    _handle_brd(sp, v[i], depth, i);
                }
                if (max_width >= 2 && v.size() >= 2) {
                    _handle_brd(sp, v.back(), depth, v.size() - 1);
                }
            }
        }
    }

    const size_t max_depth;
    const size_t max_width;
    const bool randomize;
    const Clock::time_point run_until;
    
    std::vector<board_states_generator> stack;

    const bool enable_cache;
    Cache boards_cache;

    size_t next_total_boards;
    bool running;

    stats sts;
    
    std::vector<std::vector<size_t>> random_indexes;

    const size_t boards_count_step = 1000000;
};


std::vector<board_state_t> do_bfs_level(const std::vector<board_state_t>& boards, size_t depth, stats& sts)
{
    std::vector<board_state_t> next_boards; 
    _board_states_generator g(next_boards);

    next_boards.reserve(boards.size() * 8); // 8 - empirical multiplier

    for (const auto& brd : boards) {
        size_t w = g.gen_next_states(brd);
        sts.consume_level_width(w, depth);
    }

    return next_boards;
}

void rotate_level(std::vector<board_state_t>& boards)
{
    for (auto& b : boards) {
        b = rotate(b);
    }
}

template<typename T>
std::vector<std::vector<T>> split(const std::vector<T>& v, size_t num_chunks)
{
    std::vector<std::vector<T>> r;
    r.reserve(num_chunks);

    auto beg = v.begin();
    auto en = v.end();

    while (num_chunks > 0) {
        size_t left = en - beg;
        if (left == 0) {
            break;
        }
        size_t len = left / num_chunks;
        if (len == 0) {
            break;
        }
        r.emplace_back(beg, beg + len);
        beg += len;
        num_chunks--;
    }

    return r;
}

void debug_split()
{
    std::vector<int> src(25);
    std::iota(src.begin(), src.end(), 0);

    for (int i = 1; i <= 10; i++) {
        auto v = split(src, i);
        printf("%lu: ", v.size());
        for (const auto& x : v) {
            printf("%lu, ", x.size());
        }  
        printf("\n");
    }
}

template<class Cache>
struct MTDFS
{
    MTDFS(size_t num_threads, const search_config_t& cfg, size_t min_initial_boards_per_thread = 20) :
        min_initial_boards_per_thread(min_initial_boards_per_thread),
        workers(num_threads, cfg)
    {}

    // Initial BFS step does not take into account search configuration 
    void do_search(const board_state_t& brd)
    {
        printf("Multi-thread DFS\n");

        stats sts;
        auto started = Clock::now();

        std::vector<std::future<dfs_result_t>> results;

        std::vector<board_state_t> level{brd};
        size_t depth = 0;

        size_t min_level_size = workers.size() * min_initial_boards_per_thread;

        while (level.size() < min_level_size) {
            level = do_bfs_level(level, depth, sts);
            depth++;
            rotate_level(level);
        }
        printf("initial BFS finished\ndepth: %lu\nboards: %lu\n", depth, level.size());

        auto splitted_level = split(level, workers.size());

        size_t i = 0;
        for (auto& v : splitted_level) {
            results.emplace_back(
                std::async(
                    std::launch::async,
                    workers[i].get_callable(std::move(v), depth)
                )
            );
            i++;
        }

        bool completed = true;
        for (auto& f : results) {
            auto r = f.get();
            sts += std::get<0>(r);
            completed = completed & std::get<1>(r);
        }

        //TODO: progress

        printf("\n%s\n", completed ? "Completed!" : "Terminated.");
        sts.print(started, workers.size());
    }

private:
    size_t min_initial_boards_per_thread;

    std::vector<DFS_worker<Cache>> workers;
};


const board_state_t initial_board = {board_side_t{0, 0x0FFF}, board_side_t{0, 0xFFF00000}};

void debug_step(board_state_t brd)
{
    board_states_generator g;
    auto v = g.gen_next_states(brd);
    int count = 0;
    for (auto after : v) {
        printf("\n\n  ======================= %d ======================\n", count++);
        print(brd);
        print(after);
    }
}

void debug_depth(board_state_t brd, size_t depth)
{
    board_states_generator g;

    size_t cnt = 0;
    while (cnt <= depth) {
        printf("\n  move: %lu\n", cnt);
        print(brd);
        auto v = g.gen_next_states(brd);
        cnt++;
        if (v.size() > 0) {
            brd = v.front();
            printf("\n  move: %lu\n", cnt);
            print(brd);

            v = g.gen_next_states(rotate(brd));
            cnt++;
            if (v.size() > 0) {
                brd = rotate(v.front());
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

void debug()
{
    int i = 0;
    for (auto pos : pos_table) {
        printf("%2d : {%d,%d}\n", i++, pos.first, pos.second);
    }

    for (int Y = 0; Y < 8; Y += 2) {
        int y = Y;
        for (int x = 0; x < 8; x++) {
            printf("%2d:%08x:{%d,%d} -> ", pos2index({x, y}), pos2bitmap({x, y}), x, y);
            X_t _x(x, y);
            X_t moves[4] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
            for (auto move : moves) {
                X_t t = _x + move;
                if (is_valid(t)) {
                    printf("%2d:%08x:{%d,%d}, ", pos2index(t), pos2bitmap(t), t.first, t.second);
                }
            }

            printf("\n");
            
            y += (x % 2 == 0) ? 1 : -1;
        }
    }

    printf("fwd_moves:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.fwd_moves[i];
        int j = 0;
        for (uint32_t v = a[j]; v != 0 && j < a.size(); v = a[++j]) {
            printf("%08x, ", v);
        }
        printf("\n");
    }
    printf("captures:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.captures[i];
        int j = 0;
        for (auto v = a[j]; std::get<0>(v) != 0 && j < a.size(); v = a[++j]) {
            printf("%08x_%d, ", std::get<0>(v), std::get<1>(v));
        }
        printf("\n");
    }
    printf("king_moves:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.king_moves[i];
        for (const auto& b : a) {
            int j = 0;
            printf("[");
            for (uint32_t v = b[j]; v != 0 && j < b.size(); v = b[++j]) {
                printf("%s%08x", j == 0 ? "" : ", ", v);
            }
            printf("], ");
        }
        printf("\n");
    }
    printf("king_captures:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.king_captures[i];
        for (const auto& b : a) {
            int j = 0;
            printf("[");
            for (auto v = b[j]; v.first != 0 && j < b.size(); v = b[++j]) {
                printf("%s{%08x, [", j == 0 ? "" : ", ", v.first);
                int k = 0;
                for (auto dst : v.second) {
                    if (dst < 0) {
                        break;
                    }
                    printf("%s%d", k++ == 0 ? "" : ", ", dst);
                }
                printf("]}");
            }
            printf("], ");
        }
        printf("\n");
    }

    printf("fwd_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.fwd_move_masks[i]);
    }
    printf("capture_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_move_masks[i]);
    }
    printf("capture_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_masks[i]);
    }
    printf("king_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.king_move_masks[i]);
    }


    for (int index = 0; index < 32; index++) {
        auto x = index2pos(index);
        printf("%d,%d\n", x.first, x.second);
    }
    for (moves_iter<4> it({5, 7}, short_captures); it; ++it) {
        printf("%d,%d\n", it->first, it->second);
    }



    board_state_t brd = {board_side_t{0x0108, 0x0FFF}, board_side_t{0x24A00000, 0xFFF00000}};
    print(brd);
    print(rotate(brd));

    debug_step(initial_board);
    printf("\n\n\n");

    debug_step({board_side_t{0, 0x00002000}, board_side_t{0, 0x00040000}});
    printf("\n\n\n");

    printf("==================================================================\n");
    debug_depth(initial_board, 100);
}

#if 0
constexpr int numberOfSetBits(uint32_t i)
{
     i = i - ((i >> 1) & 0x55555555);
     i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
     return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

int max(int a, int b)
{
    return a > b ? a : b;
}
int min(int a, int b)
{
    return a < b ? a : b;
}

constexpr auto make_hist()
{
    std::array<std::array<int, 32>, 32> hist{{0}};
    size_t m = 0;
    for (uint32_t i = 1; i != 0; i++) {
        int n = numberOfSetBits(i);
        if (n <= 1 && i != 1) {
            hist[m+1] = hist[m];
            m++;
        }
        hist[m][n-1]++;
    }
    return hist;
}

void calc_all()
{
    const std::array<std::array<int, 32>, 32> hist = make_hist();

    for (int width = 1; width <= 32; width++) {
        printf("\n-------- width=%d --------\n", width);
        for (int set_bits = 1; set_bits <= 32; set_bits++) {
            printf("%2d : %d\n", set_bits, hist[width-1][set_bits-1]);
        }
    }

    double total = 0;
    for (int occupied = 2; occupied <= 24; occupied++) {
        size_t occ_vars = hist[31][occupied-1];
        printf("\noccupied=%d: %ld * (", occupied, occ_vars);

        bool w_start = true;
        double w_total = 0;
        for (int whites = max(occupied-12, 1); whites <= min(occupied-1, 12); whites++) {

            if (w_start) {
                w_start = false;
            } else {
                printf(" + ");
            }

            size_t w_vars = hist[occupied-1][whites];
            printf("%ld * (", w_vars);

            int blacks = occupied - whites;
            bool wk_start = true;
            double wk_total = 0;
            for (int white_kings = 0; white_kings <= whites; white_kings++) {

                if (wk_start) {
                    wk_start = false;
                } else {
                    printf(" + ");
                }

                size_t wk_vars = 1;
                if (white_kings > 0) {
                    wk_vars = hist[whites-1][white_kings-1];
                }
                printf("%ld * (", wk_vars);

                bool bk_start = true;
                double bk_total = 0;
                for (int black_kings = 0; black_kings <= blacks; black_kings++) {

                    if (bk_start) {
                        bk_start = false;
                    } else {
                        printf(" + ");
                    }

                    size_t bk_vars = 1;
                    if (black_kings > 0) {
                        bk_vars = hist[blacks-1][black_kings-1];
                    }
                    printf("%ld", bk_vars);
                    bk_total += bk_vars;
                }
                printf(")");

                wk_total += wk_vars * bk_total;
            }
            printf(")");

            w_total += w_vars * wk_total;
        }
        printf(")\n");

        printf("%lu * %lf = %lf\n", occ_vars, w_total, occ_vars * w_total);
        total += occ_vars * w_total;
        printf("total=%lf\n", total);
    }

    printf("total=%lf\n", total/2);
}
#endif

struct readable_duration_t
{
    Clock::duration value;
    
    readable_duration_t() = default;

    readable_duration_t(const Clock::duration& d) :
        value(d)
    {}

    readable_duration_t& operator= (const Clock::duration& d)
    {
        value = d;
        return *this;
    }

    static const std::map<std::string, Clock::duration> units;

    friend std::ostream& operator<<(std::ostream &out, const readable_duration_t& d)
    {
        out << d.value.count() << 's';
        return out;
    }

    friend std::istream& operator>>(std::istream &in, readable_duration_t& d)
    {
        float v;
        std::string u;
        in >> v;
        
        if (!in.eof()) {
            in >> u;
        }

        Clock::duration unit = 1s;

        auto it = units.find(u);
        if (it != units.end()) {
            unit = it->second;
        }

        d.value = std::chrono::duration_cast<Clock::duration>(unit * v);

        return in;
    }

    static std::string all_units(const std::string& delim)
    {
        return boost::algorithm::join(units | boost::adaptors::map_keys, delim);
    }
};

const std::map<std::string, Clock::duration> readable_duration_t::units = 
{
    {"us", 1us},
    {"ms", 1ms},
    {"s", 1s},
    {"m", 1min},
    {"h", 1h},
    {"d", 24h}
};


void do_dfs_cmd()
{
    //TODO: all args to struct cfg_t
}

void signal_handler(int signum)
{
   g_running = false;
}

int main(int argc, const char* argv[])
{
    //debug();
    //calc_all();
    //debug_judy_128_set();
    //debug_split();
    //return 0;

    signal(SIGINT, signal_handler);  
    signal(SIGTERM, signal_handler); 



    size_t max_depth;
    bool verbose;
    readable_duration_t timeout{10s};
    std::string command;
    bool randomize;
    size_t max_width;
    bool cache;
    bool print_cache_hits;
    bool print_wins;
    std::string cache_impl;
    size_t n_threads;

    std::string header = "DTE - Decision Tree Explorer (Russian Draughts)\n";
    header += "\nUsage: ";
    header += argv[0];
    header += " dfs | mtdfs [options]\n";
    header += "\nCommands:\n";
    header += "  dfs - Depth-first search\n";
    header += "  mtdfs - Multi-threaded depth-first search\n";
    header += "\nOptions";

    std::string timeout_desc = "timeout, default=10s\nunits = "s + readable_duration_t::all_units(" | ") + "\ndefault unit = s";

    po::options_description visible_opts(header);
    visible_opts.add_options()
        ("help,h", "show help")
        ("max-depth,d", po::value<size_t>(&max_depth)->default_value(10), "max search depth")
        ("verbose,v", po::bool_switch(&verbose), "print all boards")
        ("timeout,t", po::value<readable_duration_t>(&timeout), timeout_desc.c_str())
        ("randomize,r", po::bool_switch(&randomize), "randomize braches iteration")
        ("max-width,w", po::value<size_t>(&max_width)->default_value(0), "max branches iterate, 0 - all\nwith randomize=false max-width = 1 | 2 | 3")
        ("cache,c", po::bool_switch(&cache), "enable board cache and cache_hit detection")
        ("print-cache-hits,H", po::bool_switch(&print_cache_hits), "print board for cache hit case")
        ("print-wins,W", po::bool_switch(&print_wins), "print entire path for win case")
        ("cache-impl,C", po::value<std::string>(&cache_impl)->default_value("judy"), "cache implementation: std | dense | judy")
        ("threads,j", po::value<size_t>(&n_threads)->default_value(1), "number of threads, for mtdfs")
    ;

    po::options_description hidden_opts;
    hidden_opts.add_options()
        ("command", po::value<std::string>(&command), "command")
    ;

    po::options_description cmdline_opts;
    cmdline_opts.add(visible_opts).add(hidden_opts);

    po::positional_options_description pos;
    pos.add("command", -1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(cmdline_opts).positional(pos).run(), vm);
        po::notify(vm);
    } catch(const po::error& e) {
        std::cerr << "Couldn't parse command line arguments:" << std::endl;
        std::cerr << e.what() << std::endl << std::endl;
        std::cerr << visible_opts << std::endl;
        return 1;
    }

    if (vm.count("help")) {
        std::cout << visible_opts << std::endl;
        return 0;
    }

    if (command == "dfs") {
        if (cache_impl == "std") {

            DFS<std_cache> x(max_depth, verbose, timeout.value,
                             max_width, randomize, cache, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else if (cache_impl == "dense") {

            DFS<dense_cache> x(max_depth, verbose, timeout.value,
                               max_width, randomize, cache, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else if (cache_impl == "judy") {

            DFS<judy_cache> x(max_depth, verbose, timeout.value,
                              max_width, randomize, cache, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else {
            std::cerr << "unknown cache implementatin: \"" << cache_impl << "\"" << std::endl;
            std::cerr << visible_opts << std::endl;
        }

    } else if (command == "mtdfs") {

        search_config_t scfg{
            max_depth,
            Clock::now() + timeout.value,
            max_width,
            randomize,
            cache
        };

        if (cache_impl == "std") {

            MTDFS<std_cache> x(n_threads, scfg);
            x.do_search(initial_board);

        } else if (cache_impl == "dense") {

            MTDFS<dense_cache> x(n_threads, scfg);
            x.do_search(initial_board);

        } else if (cache_impl == "judy") {

            MTDFS<judy_cache> x(n_threads, scfg);
            x.do_search(initial_board);

        } else {
            std::cerr << "unknown cache implementatin: \"" << cache_impl << "\"" << std::endl;
            std::cerr << visible_opts << std::endl;
        }

    } else {
        if (vm.count("command") == 0) {
            std::cerr << "command is required" << std::endl;
        } else {
            std::cerr << "Unknown command" << std::endl;
        }
        std::cerr << visible_opts << std::endl;
        return 1;
    }

    return 0;
}











