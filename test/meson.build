


cvc4_dep = declare_dependency(link_args: '-lcvc4')
assert(cxx.has_header('cvc4/cvc4.h'), 'CVC4 headers required, please install e.g. "apt install libcvc4-dev"')

gnu_mp_dep = declare_dependency(link_args: '-lgmp')

test_deps = [cvc4_dep, gnu_mp_dep]

testapp = executable('testapp', 'tests.cc', dependencies: test_deps)



test('name of test', testapp)

