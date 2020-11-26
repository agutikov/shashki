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
#include <initializer_list>
#include <random>
#include <unordered_set>
#include <type_traits>
#include <thread>
#include <future>
#include <csignal>

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/functional/hash.hpp>
#include <boost/program_options.hpp>

#include <google/dense_hash_set>

#include "judy_128_set.h"
#include "utils.h"
#include "draughts.h"


namespace po = boost::program_options;

using namespace std::string_literals;

using namespace std::literals::chrono_literals;
using Clock = std::chrono::system_clock;





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
    const bool randomize;
    const size_t max_width;
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
                //TODO: do same as split() from utils.h does

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
    const bool randomize;
    const size_t max_width;
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

        //TODO: fater destroy cache
    }

private:
    size_t min_initial_boards_per_thread;

    std::vector<DFS_worker<Cache>> workers;
};


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

    std::string header = "DTS - Decision Tree Statistics (Russian Draughts)\n";
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











