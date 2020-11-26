#pragma once

#include <cstdio>
#include <array>
#include <utility>
#include <vector>
#include <algorithm>

// 2-dimentional coordinate of draughts item on board
// each coordinate starts from 0
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

inline const std::array<X_t, 32> pos_table = gen_pos_table();

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

inline const std::array<X_t, 2> fwd_directions{X_t{-1,  1}, X_t{1,  1}};
inline const std::array<X_t, 4> all_directions = product(std::array<int, 2>{-1, 1});

inline const std::array<X_t, 4> short_captures_dst = product(std::array<int, 2>{-2, 2});
inline const std::array<X_t, 4> short_captures = product(std::array<int, 2>{-1, 1});

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

inline const std::array<X_t, 28> long_moves = product(all_directions, std::array<int, 7>{1, 2, 3, 4, 5, 6, 7});
inline const std::array<X_t, 24> long_captures = product(all_directions, std::array<int, 6>{1, 2, 3, 4, 5, 6});
inline const std::array<X_t, 24> long_jumps = product(all_directions, std::array<int, 6>{2, 3, 4, 5, 6, 7});

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
            for (size_t c_index = 0; c_index < dir_zip.size(); c_index++) {
                auto c = dir_zip[c_index];
                auto capture = x + c.second;
                auto min_dst = x + c.first;
                if (is_valid(min_dst)) {
                    out_dir[c_i] = std::pair<uint32_t, std::array<int, 6>>{pos2bitmap(capture), {-1}};
                    out_dir[c_i].second.fill(-1);

                    // and fill possible jump destinations for this capture
                    int j_i = 0;
                    for (size_t j_index = c_index; j_index < dir_zip.size(); j_index++) {
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

inline const struct tables_t
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

inline const board_state_t initial_board = {board_side_t{0, 0x0FFF}, board_side_t{0, 0xFFF00000}};

inline const int format_table[8][8] = {
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
inline const uint8_t bit_reverse_table_256[] = 
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
