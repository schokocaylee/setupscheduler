cmake_minimum_required(VERSION 3.25)
project(setupscheduler)
set(CMAKE_C_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

add_executable(setupscheduler main.c json.c prepare.c basic.c preempt.c split.c nonpreempt.c search.c generate.c validate.c jump.c test.c)
target_compile_options(setupscheduler PUBLIC -O3 -D_GNU_SOURCE)
target_link_libraries(setupscheduler PUBLIC m PkgConfig::CJSON)
target_link_options(setupscheduler PUBLIC -O3)
