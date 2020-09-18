

set(
    TINYHTTPD_SOURCE
    ${PROJECT_SOURCE_DIR}/source/main.cpp
)

# add the executable
include_directories(${PROJECT_SOURCE_DIR}/contrib/libuv/include)
add_executable(tinyhttpd ${TINYHTTPD_SOURCE})
target_link_libraries(tinyhttpd uv_a)