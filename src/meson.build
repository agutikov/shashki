

dts_sources = files('main.cc')


engine_sources = ['wrapper.cc']
engine_lib = shared_library('engine', engine_sources,
                        include_directories : inc,
                        dependencies : external_deps)
engine_dep = declare_dependency(link_with: engine_lib, include_directories: inc)

all_deps = [engine_dep] + external_deps

executable('dts', dts_sources, dependencies: external_deps)

executable('count-boards', 'count_boards.cc')

executable('debug', 'debug.cc', dependencies: all_deps)

executable('example', 'example.c', dependencies: [engine_dep])
