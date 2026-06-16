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

# Utilities
if (NOT EMSCRIPTEN)
    add_executable(img_conv x/examples/libpc98/img_conv.cc)
    target_link_libraries(img_conv libpc98)
    add_executable(snd_conv x/examples/libpc98/snd_conv.cc)
    target_link_libraries(snd_conv libpc98)
endif()


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


# palcyc demo
add_executable(palcyc
    x/examples/palcyc/main.cc
)
target_link_libraries(palcyc libpc98)
if (EMSCRIPTEN)
    target_link_options(palcyc PRIVATE
        --embed-file "${CMAKE_SOURCE_DIR}/palcyc@/palcyc"
        --shell-file "${CMAKE_SOURCE_DIR}/web_sources/shell_minimal.html"
    )
endif()
