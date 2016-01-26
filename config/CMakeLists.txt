# primary library
add_library( config
    src/Parameters.cpp
)
target_link_libraries( config
    jsoncpp
)
add_executable(testParameters test/testParameters.cpp)
target_link_libraries(testParameters logger config rpc  )
