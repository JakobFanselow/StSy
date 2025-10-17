cmake_minimum_required(VERSION 3.16)
project(tsip)

set(CMAKE_CXX_STANDARD 17)


include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

FILE(GLOB TSIP_DEPENDS src/common/*.hpp src/common/*.cpp src/lib/*.hpp src/lib/*.cpp)



find_package(Boost REQUIRED)
find_package(fmt REQUIRED)


add_library(tsip_static STATIC src/main.cpp)
add_library(tsip_shared SHARED ${TSIP_DEPENDS})
add_executable(client src/client/main.cpp)
add_executable(server src/server/main.cpp)
add_executable(tests tests/ip.cpp)

target_link_libraries(tsip_static PRIVATE fmt::fmt)
target_link_libraries(tsip_shared PRIVATE fmt::fmt)
target_link_libraries(server PRIVATE fmt::fmt tsip_shared)
target_link_libraries(client PRIVATE fmt::fmt tsip_shared)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain tsip_shared fmt::fmt)