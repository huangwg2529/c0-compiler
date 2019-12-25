# CMake generated Testfile for 
# Source directory: /root/share/c0
# Build directory: /root/share/c0/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(all_test "cc0_test")
subdirs("3rd_party/argparse")
subdirs("3rd_party/fmt")
subdirs("3rd_party/catch2")
