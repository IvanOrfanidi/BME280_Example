cmake_minimum_required(VERSION 3.10)

project(bme280_example)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(
    -Werror
    -Wall
    -Wextra
    -Wpedantic
)

file(GLOB_RECURSE SOURCES
    source/*.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(Threads)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)
