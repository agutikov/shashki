#include <cstdio>
#include <iostream>
#include <csignal>

#include <boost/program_options.hpp>

#include "utils.h"
#include "draughts.h"
#include "dfs.h"
#include "mtdfs.h"

using namespace std::string_literals;

namespace po = boost::program_options;



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
    readable_duration_t<Clock> timeout{10s};
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

    std::string timeout_desc = "timeout, default=10s\nunits = "s + readable_duration_t<Clock>::all_units(" | ") + "\ndefault unit = s";

    po::options_description visible_opts(header);
    visible_opts.add_options()
        ("help,h", "show help")
        ("max-depth,d", po::value<size_t>(&max_depth)->default_value(10), "max search depth")
        ("verbose,v", po::bool_switch(&verbose), "print all boards")
        ("timeout,t", po::value<decltype(timeout)>(&timeout), timeout_desc.c_str())
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

    search_config_t scfg{
        max_depth,
        Clock::now() + timeout.value,
        max_width,
        randomize,
        cache
    };

    if (command == "dfs") {
        if (cache_impl == "std") {

            DFS<std_cache> x(scfg, verbose, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else if (cache_impl == "dense") {

            DFS<dense_cache> x(scfg, verbose, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else if (cache_impl == "judy") {

            DFS<judy_cache> x(scfg, verbose, print_wins, print_cache_hits);
            x.do_search(initial_board);

        } else {
            std::cerr << "unknown cache implementation: \"" << cache_impl << "\"" << std::endl;
            std::cerr << visible_opts << std::endl;
        }

    } else if (command == "mtdfs") {

        if (cache_impl == "std") {

            MTDFS<DFS<std_cache, false>> x(n_threads, scfg);
            x.do_search(initial_board);

        } else if (cache_impl == "dense") {

            MTDFS<DFS<dense_cache, false>> x(n_threads, scfg);
            x.do_search(initial_board);

        } else if (cache_impl == "judy") {

            MTDFS<DFS<judy_cache, false>> x(n_threads, scfg);
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
