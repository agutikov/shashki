
#include <cstdio>
#include <array>
#include <utility>
#include <vector>
#include <algorithm>
#include <tuple>



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

const X_t& index2pos(int i)
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

const std::array<X_t, 2> forward_moves{X_t{-1,  1}, X_t{1,  1}};

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

const std::array<X_t, 28> long_moves = product(product(std::array<int, 2>{-1, 1}), std::array<int, 7>{1, 2, 3, 4, 5, 6, 7});

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
    std::array<std::array<std::tuple<uint32_t, uint32_t, int>, L>, 32> r{{std::tuple<uint32_t, uint32_t, int>{0,0,0}}};

    for (int index = 0; index < 32; index++) {
        auto x = index2pos(index);
        auto& out_a = r[index];
        out_a.fill({0,0,0});

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
                out_a[i++] = std::tuple<uint32_t, uint32_t, int>(pos2bitmap(dst), pos2bitmap(cap), pos2index(dst));
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
                auto z = x + move;
                if (is_valid(z)) {
                    out_dir[i++] = pos2bitmap(z);
                }
            }
        }
    }

    return r;
}

const struct tables_t
{
    std::array<std::array<uint32_t, 2>, 32> fwd_moves = gen_moves<2>(forward_moves);
    std::array<uint32_t, 32> fwd_move_masks = moves_mask<2>(forward_moves);

    // first is move, second is capture, third is move destination index
    std::array<std::array<std::tuple<uint32_t, uint32_t, int>, 4>, 32> captures = gen_captures<4>(short_captures_dst, short_captures);
    std::array<uint32_t, 32> capture_move_masks = moves_mask<4>(short_captures_dst);
    std::array<uint32_t, 32> capture_masks = moves_mask<4>(short_captures);

    std::array<std::array<std::array<uint32_t, 7>, 4>, 32> king_moves = gen_king_moves();
    std::array<uint32_t, 32> king_move_masks = moves_mask<28>(long_moves);
} tables;


struct board_side_t
{
    uint32_t kings = 0;
    uint32_t items = 0;
};

struct board_state_t
{
    std::array<board_side_t, 2> sides;
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
    printf("\n%s\n%s\n", cols, line);
    for (const auto& f : format_table) {
        printf("%d |", row);
        for (int i : f) {
            if (i == -1) {
                printf("     |");
            } else {
                uint32_t mask = index2bitmap(i);
                const char* c = s.sides[0].kings & mask ? " (O) " :
                         s.sides[0].items & mask ? "  o  " :
                         s.sides[1].kings & mask ? " }x{ " :
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

board_side_t reverse(board_side_t b)
{
    return {reverse(b.kings), reverse(b.items)};
}

board_state_t rotate(board_state_t s)
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





struct board_states_generator
{
    const std::vector<board_state_t>& gen_next_states(const board_state_t& s)
    {
        cur_state = s;
        states.clear();
        occupied = cur_state.sides[0].items | cur_state.sides[1].items;

        gen_states();

        return states;
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
        
        //printf("%d %08X %08X %08X\n", item_index, moves_mask, available_dst, may_be_captured);

        // not allow capture same items multiple times
        may_be_captured &= ~captured;

        //printf("%08X\n", may_be_captured);
        
        if (may_be_captured == 0 || available_dst == 0) {
            // capture sequence completed
            if (captured != 0) {
                // do capture
                board_state_t next_state = do_capture(state, captured);
                //print(next_state);

                // save new state if it is final
                states.push_back(next_state);
                return 1;
            } else {
                return 0;
            }
        }

        size_t saved_states = 0;
        for (auto [move, capture, dst_index] : tables.captures[item_index]) {
            //printf("%08X %08X %d\n", move, capture, dst_index);

            if (move == 0) {
                break;
            }
            // branching possible capture moves, follow moves
            if ((capture & may_be_captured) && (move & available_dst)) {
                // do move
                board_state_t next_state = do_move(state, index2bitmap(item_index), move);
                //print(next_state);

                // try continue capturing
                saved_states += next_item_captures(next_state, dst_index, captured | capture);
            }
        }

        return saved_states;
    }

    size_t next_item_moves(int item_index)
    {
        uint32_t moves_mask = tables.fwd_move_masks[item_index];
        uint32_t available_dst = (moves_mask & occupied) ^ moves_mask;

        //printf("%d %08X %08X %08X\n", item_index, occupied, moves_mask, available_dst);

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
                states.push_back(next_state);
                saved_states++;
            }
        }
        return saved_states;
    }

    size_t next_king_captures(const board_state_t& state, int item_index, uint32_t captured)
    {
        return 0;
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
                    states.push_back(next_state);
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

    void gen_states()
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
            return;
        }

        for (int item_index = 0; item_index < 32; item_index++) {
            uint32_t item = index2bitmap(item_index);
            if (item & player.kings) {
                next_king_moves(item_index);
            } else if (item & player.items) {
                next_item_moves(item_index);
            }
        }
    }

    uint32_t occupied;

    board_state_t cur_state;
    std::vector<board_state_t> states;
};




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

    size_t count = 0;
    while (count <= depth) {
        printf("\n  STEP=%lu\n", count);
        print(brd);
        auto v = g.gen_next_states(brd);
        if (v.size() > 0) {
            brd = v.front();
            print(brd);

            v.clear();
            v = g.gen_next_states(rotate(brd));
            if (v.size() > 0) {
                brd = rotate(v.back());
            } else {
                break;
            }
        } else {
            break;
        }
        count++;
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

    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.fwd_moves[i];
        int j = 0;
        for (uint32_t v = a[j]; v != 0 && j < a.size(); v = a[++j]) {
            printf("%08x, ", v);
        }
        printf("\n");
    }
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.captures[i];
        int j = 0;
        for (auto v = a[j]; std::get<0>(v) != 0 && j < a.size(); v = a[++j]) {
            printf("%08x_%08x_%02d, ", std::get<0>(v), std::get<1>(v), std::get<2>(v));
        }
        printf("\n");
    }
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

    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.fwd_move_masks[i]);
    }
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_move_masks[i]);
    }
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_masks[i]);
    }
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

    debug_step({board_side_t{0, 0x0FFF}, board_side_t{0, 0xFFF00000}});
    printf("\n\n\n");

    debug_step({board_side_t{0, 0x00002000}, board_side_t{0, 0x00040000}});
    printf("\n\n\n");

    printf("==================================================================\n");
    debug_depth({board_side_t{0, 0x0FFF}, board_side_t{0, 0xFFF00000}}, 100);
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


int main()
{
    debug();


    //calc_all();


    return 0;
}











