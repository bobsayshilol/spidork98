cmake_minimum_required(VERSION 3.28)
project(spidork98)

include(web_sources/compiler_flags.cmake)

# Really this should be PC98_BUILD=0, but I don't want to touch the Makefiles
add_compile_definitions(WEB_BUILD)

find_package(SDL3 REQUIRED COMPONENTS SDL3-static)


# The main framework
add_library(libpc98
    # Common
    x/examples/libpc98/images.cc
    x/examples/libpc98/images2.cc
    x/examples/libpc98/logs.cc
    x/examples/libpc98/memory.cc
    x/examples/libpc98/profile.cc
    x/examples/libpc98/sound.cc

    # Non-PC98 bits
    web_sources/web_common.cc
    web_sources/web_funcs.cc
    web_sources/web_gpu.cc
    web_sources/web_pcm.cc
)
target_include_directories(libpc98 PUBLIC
    x/examples/libpc98
    web_sources
)
target_link_libraries(libpc98 PRIVATE SDL3::SDL3-static)


# AAA game
add_executable(aaa
    x/examples/aaa/loading.cc
    x/examples/aaa/main.cc
    x/examples/aaa/mn_creds.cc
    x/examples/aaa/mn_game.cc
    x/examples/aaa/mn_lvls.cc
    x/examples/aaa/mn_main.cc
    x/examples/aaa/mn_splsh.cc
    x/examples/aaa/pal_fade.cc
)
target_link_libraries(aaa libpc98)
if (EMSCRIPTEN)
    target_link_options(aaa PRIVATE
        --embed-file "${CMAKE_SOURCE_DIR}/aaa@/aaa"
        --shell-file "${CMAKE_SOURCE_DIR}/web_sources/shell_minimal.html"
    )
endif()


# Puzzles game
add_executable(puzzles
    # Common
    x/examples/puzzles/puzzles/combi.c
    x/examples/puzzles/puzzles/divvy.c
    x/examples/puzzles/puzzles/draw-ply.c
    x/examples/puzzles/puzzles/drawing.c
    x/examples/puzzles/puzzles/dsf.c
    x/examples/puzzles/puzzles/findloop.c
    x/examples/puzzles/puzzles/grid.c
    x/examples/puzzles/puzzles/hat.c
    x/examples/puzzles/puzzles/latin.c
    x/examples/puzzles/puzzles/laydomin.c
    x/examples/puzzles/puzzles/loopgen.c
    x/examples/puzzles/puzzles/malloc.c
    x/examples/puzzles/puzzles/matching.c
    x/examples/puzzles/puzzles/midend.c
    x/examples/puzzles/puzzles/misc.c
    x/examples/puzzles/puzzles/penrose.c
    x/examples/puzzles/puzzles/pen-leg.c
    x/examples/puzzles/puzzles/ps.c
    x/examples/puzzles/puzzles/random.c
    x/examples/puzzles/puzzles/sort.c
    x/examples/puzzles/puzzles/spectre.c
    x/examples/puzzles/puzzles/tdq.c
    x/examples/puzzles/puzzles/tree234.c
    x/examples/puzzles/puzzles/version.c

    # Games
    x/examples/puzzles/puzzles/mines.c

    # Frontend
    x/examples/puzzles/fe98.cc
)
target_include_directories(puzzles PUBLIC
    x/examples/puzzles/puzzles
)
target_compile_definitions(puzzles PUBLIC COMBINED FE98_BUILD)
target_link_libraries(puzzles libpc98)
if (EMSCRIPTEN)
    target_link_options(puzzles PRIVATE
        --shell-file "${CMAKE_SOURCE_DIR}/web_sources/shell_minimal.html"
    )
endif()
