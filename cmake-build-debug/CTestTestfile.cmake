# CMake generated Testfile for 
# Source directory: D:/share/c0
# Build directory: D:/share/c0/cmake-build-debug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(all_test "cc0_test")
set_tests_properties(all_test PROPERTIES  _BACKTRACE_TRIPLES "D:/share/c0/CMakeLists.txt;76;add_test;D:/share/c0/CMakeLists.txt;0;")
subdirs("3rd_party/argparse")
subdirs("3rd_party/fmt")
subdirs("3rd_party/catch2")
