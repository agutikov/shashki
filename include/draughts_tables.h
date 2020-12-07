#pragma once

#include "draughts_types.h"


template <size_t L1, size_t L2>
constexpr auto product(const std::array<brd_2d_vector_t, L1>& x, const std::array<int, L2>& m)
{
    std::array<brd_2d_vector_t, L1*L2> r;
    size_t i = 0;
    for (brd_2d_vector_t x_i : x) {
        for (int m_j : m) {
            r[i++] = x_i * m_j;
        }
    }
    return r;
}


inline const std::array<brd_2d_vector_t, 2> fwd_directions{up_left, up_right};
inline const std::array<brd_2d_vector_t, 4> all_directions{up_left, up_right, down_left, down_right};

inline const int item_jump_distance = 2;

inline const std::array<int, 7> king_move_distances{1, 2, 3, 4, 5, 6, 7};
inline const std::array<int, 6> king_capture_distances{1, 2, 3, 4, 5, 6};
inline const std::array<int, 6> king_jump_distances{2, 3, 4, 5, 6, 7};



template<size_t L>
constexpr auto gen_moves(const std::array<brd_2d_vector_t, L>& moves)
{
    std::array<std::array<brd_item_t, L>, 32> r{{}};

    for (brd_index_t index = 0; index; ++index) {
        auto start_position = brd_2d_vector_t(index);

        size_t i = 0;
        for (const auto& move : moves) {
            auto dest = start_position + move;
            if (dest) {
                r[index.index][i++] = brd_item_t(dest);
            }
        }
    }

    return r;
}

template<size_t L>
constexpr auto gen_masks(const std::array<brd_2d_vector_t, L>& moves)
{
    std::array<brd_map_t, 32> r{{}};

    for (brd_index_t index = 0; index; ++index) {
        auto start_position = brd_2d_vector_t(index);

        for (const auto& move : moves) {
            auto dest = start_position + move;
            if (dest) {
                r[index.index] += brd_item_t(dest);
            }
        }
    }

    return r;
}

template<size_t L>
constexpr auto gen_captures(const std::array<brd_2d_vector_t, L>& captures)
{
    std::array<std::array<std::pair<brd_item_t, brd_index_t>, L>, 32> r{{std::pair<brd_item_t, brd_index_t>{{}, {}}}};

    for (brd_index_t index = 0; index; ++index) {
        auto start_position = brd_2d_vector_t(index);

        size_t i = 0;
        for (const auto& c : captures) {
            brd_2d_vector_t cap = start_position + c;
            brd_2d_vector_t dst = start_position + (c * item_jump_distance);
            if (dst) {
                r[index.index][i++] = std::pair<brd_item_t, brd_index_t>(cap, dst);
            }
        }
    }

    return r;
}

template<size_t L1, size_t L2>
constexpr auto gen_king_moves(const std::array<brd_2d_vector_t, L1>& directions, const std::array<int, L2>& distances)
{
    std::array<std::array<std::array<brd_item_t, L2>, L1>, 32> r{{{}}};

    for (brd_index_t index = 0; index; ++index) {
        auto start_position = brd_2d_vector_t(index);

        for (size_t dir_i = 0; dir_i < L1; dir_i++){

            auto direction = directions[dir_i];

            size_t dist_i = 0;
            for (int distance : distances) {
                auto dst = start_position + (direction * distance);
                if (dst) {
                    r[index.index][dir_i][dist_i++] = brd_item_t(dst);
                }
            }
        }
    }

    return r;
}

template<size_t L1, size_t L2>
constexpr auto gen_king_captures(const std::array<brd_2d_vector_t, L1>& directions, const std::array<int, L2>& capture_distances)
{
    std::array<std::array<std::array<std::pair<brd_item_t, std::array<brd_index_t, L2>>, L2>, L1>, 32>
    r{{{std::pair<brd_item_t, std::array<brd_index_t, 6>>{{}, {{}}}}}};

    // for every item position
    for (brd_index_t index = 0; index; ++index) {
        auto start_position = brd_2d_vector_t(index);

        // for every directon from this position
        for (size_t dir_i = 0; dir_i < L1; dir_i++){

            auto direction = directions[dir_i];

            // fill possible captures for this direction
            int capture_i = 0;
            for (int capture_distance : capture_distances) {

                auto capture = start_position + (direction * capture_distance);
                auto destination = start_position + (direction * (capture_distance + 1));

                if (destination) {
                    r[index.index][dir_i][capture_i].first = brd_item_t(capture);

                    // and fill possible jump destinations for this capture
                    int jump_i = 0;
                    while (destination) {
                        r[index.index][dir_i][capture_i].second[jump_i++] = brd_index_t(destination);
                        destination += direction;
                    }

                    capture_i++;
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

//TODO: Generalization: generate same structs for items and kings, use same function for handling but different control structure

inline const struct tables_t
{
    //TODO: rename move to dst, capture_move -> jump_dst

    std::array<std::array<brd_item_t, 2>, 32> fwd_destinations = gen_moves<2>(fwd_directions);
    std::array<brd_map_t, 32> fwd_dst_masks = gen_masks<2>(fwd_directions);

    // first is capture, second is move destination index
    std::array<std::array<std::pair<brd_item_t, brd_index_t>, 4>, 32> captures = gen_captures<4>(all_directions);
    std::array<brd_map_t, 32> capture_move_masks = gen_masks<4>(product(all_directions, std::array<int, 1>{2}));
    std::array<brd_map_t, 32> capture_masks = gen_masks<4>(all_directions);


    std::array<std::array<std::array<brd_item_t, 7>, 4>, 32> king_moves = gen_king_moves<4, 7>(all_directions, king_move_distances);
    std::array<brd_map_t, 32> king_move_masks = gen_masks<28>(product(all_directions, king_move_distances));

    // item index -> 4 directions -> list of pairs (capture mask, list of possible jump destination indexes)
    std::array<std::array<std::array<std::pair<brd_item_t, std::array<brd_index_t, 6>>, 6>, 4>, 32>
    king_captures = gen_king_captures<4, 6>(all_directions, king_capture_distances);

    std::array<brd_map_t, 32> king_capture_move_masks = gen_masks<24>(product(all_directions, king_jump_distances));
    std::array<brd_map_t, 32> king_capture_masks = gen_masks<24>(product(all_directions, king_capture_distances));

} tables;

