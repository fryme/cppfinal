
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(../../include)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

link_directories(/usr/lib/i386-linux-gnu)

add_executable(final web_server.cpp web_server.h common.h server_worker.h log.h log.cpp)
target_link_libraries(final ${CMAKE_SOURCE_DIR}/lib/libev.a pthread)
#target_compile_features(final PRIVATE cxx_range_for)

add_executable(worker worker.cpp common.h log.h log.cpp http_parser.cpp http_parser.h)
target_link_libraries(worker ${CMAKE_SOURCE_DIR}/lib/libev.a pthread)
#target_compile_features(worker PRIVATE cxx_range_for)
