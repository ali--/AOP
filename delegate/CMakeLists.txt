# primary library
#target_link_libraries( delegate
#    jsoncpp
#)
add_executable(testDelegate TestDelegate.cpp)
target_link_libraries(testDelegate jsoncpp jsonrpccpp-common jsonrpccpp-server)
