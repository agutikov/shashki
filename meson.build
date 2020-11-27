
project('shashki', ['c', 'cpp'],
    version: '0.1',
    license: 'MIT',
    default_options: ['cpp_std=c++17', 'buildtype=release'])


thread_dep = dependency('threads')
boost_po_dep = dependency('boost', modules: ['program_options'])
boost_dep = dependency('boost')

cxx = meson.get_compiler('cpp')

assert(cxx.has_header('google/dense_hash_set'), 'google sparsehash required, please install e.g. "apt install libsparsehash-dev"')

judy_lib_dep = declare_dependency(link_args: '-lJudy')
assert(cxx.has_header('Judy.h'), 'judy array headers required, please install e.g. "apt install libjudy-dev"')

external_deps = [thread_dep, judy_lib_dep, boost_dep, boost_po_dep]

inc = include_directories('include')

subdir('src')

subdir('test')
