

doctest = include_directories('../doctest')

testapp = executable('testapp', 'tests.cc', include_directories: [doctest, inc])
test('test C++ API', testapp)

test_c_app = executable('test_c_app', 'tests_c.cc', include_directories: [doctest], dependencies: [engine_dep])
test('test C API', test_c_app)
