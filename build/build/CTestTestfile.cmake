# CMake generated Testfile for 
# Source directory: /Users/matanamichy/development/Load-balancer
# Build directory: /Users/matanamichy/development/Load-balancer/build/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/Users/matanamichy/development/Load-balancer/build/build/config_manager_test[1]_include.cmake")
include("/Users/matanamichy/development/Load-balancer/build/build/reactor_test[1]_include.cmake")
include("/Users/matanamichy/development/Load-balancer/build/build/connection_test[1]_include.cmake")
include("/Users/matanamichy/development/Load-balancer/build/build/backend_pool_test[1]_include.cmake")
include("/Users/matanamichy/development/Load-balancer/build/build/router_test[1]_include.cmake")
include("/Users/matanamichy/development/Load-balancer/build/build/acceptor_test[1]_include.cmake")
subdirs("_deps/nlohmann_json-build")
subdirs("_deps/googletest-build")
