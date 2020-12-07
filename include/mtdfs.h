#pragma once

#include <future>

#include "dfs.h"


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


template<class Worker>
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

        //TODO: fater destroy cache
    }

private:
    size_t min_initial_boards_per_thread;

    std::vector<Worker> workers;
};

