cmake_minimum_required(VERSION 3.16)
project(StSy)

set(CMAKE_CXX_STANDARD 17)


include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

FILE(GLOB TSIP_DEPENDS src/common/*.hpp src/common/*.cpp src/lib/*.hpp src/lib/*.cpp)



find_package(Boost REQUIRED COMPONENTS system)
find_package(fmt REQUIRED)



add_executable(server src/main.cpp)

target_link_libraries(server PRIVATE 
                      fmt::fmt 
                      Boost::boost
                      Boost::system)
