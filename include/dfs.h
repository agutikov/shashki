#pragma once

#include <chrono>
#include <unordered_set>
#include <random>
#include <numeric>

#include <boost/functional/hash.hpp>

#include <google/dense_hash_set>

#include "draughts.h"
#include "utils.h"
#include "judy_128_set.h"

using Clock = std::chrono::system_clock;



//TODO: collect stats:
// - board with max number of possible moves, depth
//
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
        for (size_t w = 0; w < level_width_hist.size(); w++) {
            printf("%2lu: %lu\n", w, level_width_hist[w]);
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



using dense_cache = google::dense_hash_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>>;
using judy_cache = judy_128_set;
using std_cache = std::unordered_set<std::pair<uint64_t, uint64_t>, boost::hash<std::pair<uint64_t, uint64_t>>>;

inline bool g_running = true;


struct search_config_t
{
    size_t max_depth;
    Clock::time_point run_until;
    size_t max_width = 0;
    bool randomize = false;
    bool cache = false;
};


typedef std::tuple<stats, bool> dfs_result_t;
typedef std::function<bool(const board_state_t&, size_t depth)> brd_callback_t;

template<class Cache, bool single_thread = true>
struct DFS
{
    DFS(const search_config_t& cfg,
        bool verbose = true,
        bool print_win_path = false,
        bool print_cache_hit_board = false,
        brd_callback_t brd_callback = nullptr)
    :
        max_depth(cfg.max_depth),
        randomize(cfg.randomize),
        max_width(cfg.max_width),
        verbose(verbose),
        run_until(cfg.run_until),
        stack(cfg.max_depth),
        enable_cache(cfg.cache),
        print_win_path(print_win_path),
        print_cache_hit_board(print_cache_hit_board),
        brd_callback(brd_callback)
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

        if (verbose || print_win_path || print_cache_hit_board) {
            boards_count_step = 1;
        } else {
            boards_count_step = 1000000;
        }
    }

    dfs_result_t do_search(const board_state_t& brd)
    {
        auto tp = Clock::to_time_t(run_until);
        std::cout << std::boolalpha
                  << "DFS, max_depth=" << max_depth
                  << ", run_until=" << std::put_time(std::localtime(&tp), "%F %T") 
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

        return {sts, running};
    }

    dfs_result_t _do_search(const board_state_t& brd)
    {
        running = true;
        next_total_boards = boards_count_step;

        _search_r(stack.data(), brd, 0);

        return {sts, running};
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
        if (sts.total_boards() < next_total_boards) {
            return;
        }
        next_total_boards += boards_count_step;

        if constexpr (single_thread) {
            if (Clock::now() > next_status_print) {
                next_status_print += status_print_period;
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - started);
                printf("elapsed: %lus, boards: %lu\n", elapsed.count(), sts.total_boards());
            }
        }

        running = Clock::now() < run_until;

        if constexpr (single_thread) {
            if (!running) {
                printf("Timeout.\n");
            }
        }

        running = running && g_running;
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
        if constexpr (single_thread) {
            if (verbose) {
                print_board(brd, depth, branch);
            }
        }

        if (enable_cache) {
            auto ins_res = boards_cache.insert(std::pair<uint64_t, uint64_t>(brd));
            if (!ins_res.second) {
                sts.cache_hit();
                if constexpr (single_thread) {
                    if (print_cache_hit_board) {
                        printf("LOOP:\n");
                        print_board(brd, depth);
                    }
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

        if constexpr (single_thread) {
            if (brd_callback && v.size() > 0) {
                for (const auto& b : v) {
                    running = brd_callback((depth % 2) ? rotate(b) : b, depth + 1);
                    if (!running) {
                        break;
                    }
                }
            }
        }

        if (v.size() == 0) {
            if constexpr (single_thread) {
                if (print_win_path) {
                    printf("%s WINS:\n", (depth % 2) ? "B" : "W");
                    size_t d = 0;
                    for (const auto& b : path) {
                        print_board(b, d++);
                    }
                    print_board(brd, depth);
                    printf("%s WINS!\n\n", (depth % 2) ? "B" : "W");
                }
            }
            return;
        }

        // go deeper
        depth++;
        sp++;

        if constexpr (single_thread) {
            if (print_win_path) {
                path.push_back(brd);
            }
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

        if constexpr (single_thread) {
            if (print_win_path) {
                path.pop_back();
            }
        }
    }

    const size_t max_depth;
    const bool randomize;
    const size_t max_width;
    const bool verbose;
    const Clock::time_point run_until;
    
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

    size_t boards_count_step;
    const Clock::duration status_print_period{2s};

    brd_callback_t brd_callback;
};
