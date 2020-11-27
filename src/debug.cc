#include <vector>
#include <algorithm>
#include <numeric>
#include <chrono>

#include "judy_128_set.h"
#include "utils.h"
#include "draughts.h"

extern "C" {
#include "draughts_c.h"
}

void debug_c()
{
    boards_t bb;
    int r = generate_moves(&bb, get_initial_board(), 1);
    printf("generate_moves: %d\n", r);

    for (size_t i = 0; i < bb.size; i++) {
        print_board(bb.data[i], 0);
    }

    printf("=========================\n");

    r = walk_all_moves(get_initial_board(), 1, print_board, 2);
    printf("walk_all_moves: %d\n", r);
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
    for (auto pos : brd_1d_to_2d_table) {
        printf("%2d : {%d,%d}\n", i++, pos.x, pos.y);
    }

    for (int Y = 0; Y < 8; Y += 2) {
        int y = Y;
        for (int x = 0; x < 8; x++) {
            printf("%2d:%08x:{%d,%d} -> ", brd_index_t(brd_2d_vector_t(x, y)).index, brd_item_t(brd_2d_vector_t(x, y)).mask, x, y);
            brd_2d_vector_t _x{x, y};
            brd_2d_vector_t moves[4] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
            for (auto move : moves) {
                brd_2d_vector_t t = _x + move;
                if (t) {
                    printf("%2d:%08x:{%d,%d}, ", brd_index_t(t).index, brd_item_t(t).mask, t.x, t.y);
                }
            }

            printf("\n");
            
            y += (x % 2 == 0) ? 1 : -1;
        }
    }

    printf("fwd_moves:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.fwd_destinations[i];
        size_t j = 0;
        for (auto v = a[j]; v && j < a.size(); v = a[++j]) {
            printf("%08x, ", v.mask);
        }
        printf("\n");
    }
    printf("captures:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.captures[i];
        size_t j = 0;
        for (auto v = a[j]; std::get<0>(v) && j < a.size(); v = a[++j]) {
            printf("%08x_%d, ", std::get<0>(v).mask, std::get<1>(v).index);
        }
        printf("\n");
    }
    printf("king_moves:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> ", i, 1 << i);
        const auto& a = tables.king_moves[i];
        for (const auto& b : a) {
            size_t j = 0;
            printf("[");
            for (auto v = b[j]; v && j < b.size(); v = b[++j]) {
                printf("%s%08x", j == 0 ? "" : ", ", v.mask);
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
            size_t j = 0;
            printf("[");
            for (auto v = b[j]; v.first && j < b.size(); v = b[++j]) {
                printf("%s{%08x, [", j == 0 ? "" : ", ", v.first.mask);
                int k = 0;
                for (auto dst : v.second) {
                    if (!dst) {
                        break;
                    }
                    printf("%s%d", k++ == 0 ? "" : ", ", dst.index);
                }
                printf("]}");
            }
            printf("], ");
        }
        printf("\n");
    }

    printf("fwd_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.fwd_dst_masks[i].mask);
    }
    printf("capture_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_move_masks[i].mask);
    }
    printf("capture_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.capture_masks[i].mask);
    }
    printf("king_move_masks:\n");
    for (int i = 0; i < 32; i++) {
        printf("%2d %08x -> %08x\n", i, 1 << i, tables.king_move_masks[i].mask);
    }


    for (brd_index_t index = 0; index; ++index) {
        auto x = brd_2d_vector_t(index);
        printf("%d,%d\n", x.x, x.y);
    }
    for (const auto& x : brd_1d_to_2d_table) {
        printf("%d,%d\n", x.x, x.y);
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


int main()
{
    debug();
    debug_judy_128_set();
    debug_split();

    debug_c();
    
    return 0;
}

