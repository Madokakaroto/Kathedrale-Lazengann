cmake_minimum_required(VERSION 3.0)

project(kath)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

find_package(Lua REQUIRED)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

set(kath_root ${CMAKE_CURRENT_SOURCE_DIR})

# test project
set(test_include_directories
	${LUA_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
	${kath_root}/include
)
include_directories(${test_include_directories})
set(test_libs ${LUA_LIBRARIES} ${Boost_LIBRARIES})
set(test_sources
	${kath_root}/src/test/test.cpp
)

add_executable(kath_test ${test_sources})
target_link_libraries(kath_test ${test_libs})
