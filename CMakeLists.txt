cmake_minimum_required(VERSION 3.20)
project(Bar3D LANGUAGES CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Build with Emscripten: emcmake cmake -B build -G Ninja")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS engine/*.cpp)
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS app/*.cpp)

add_executable(app ${ENGINE_SOURCES} ${APP_SOURCES})
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(app PRIVATE --use-port=emdawnwebgpu -Wall -Wextra)

target_link_options(app PRIVATE
  --use-port=emdawnwebgpu
  "--embed-file=${CMAKE_SOURCE_DIR}/shaders@/shaders"
  -sALLOW_MEMORY_GROWTH=1
  -sENVIRONMENT=web
  -sEXPORTED_RUNTIME_METHODS=[]
  # Used from EM_ASM blocks (engine/audio.cpp).
  "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToUTF8,$UTF8ToString"
  # Used by the image decoder (engine/fetch.cpp).
  "-sEXPORTED_FUNCTIONS=_main,_malloc,_free"
)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_link_options(app PRIVATE -O3 --closure=1)
else()
  target_link_options(app PRIVATE -g -sASSERTIONS=2)
endif()

# Re-link when shaders change (they are embedded into the binary).
file(GLOB SHADER_FILES CONFIGURE_DEPENDS shaders/*.wgsl)
set_property(TARGET app APPEND PROPERTY LINK_DEPENDS ${SHADER_FILES})

# Place the web page next to the compiled app.js / app.wasm.
add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/web $<TARGET_FILE_DIR:app>/site
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:app>/app.js $<TARGET_FILE_DIR:app>/app.wasm $<TARGET_FILE_DIR:app>/site
)
# Streamed assets (music, models) are served as separate files. Copied on
# every build, so a rebuilt model shows up even when no C++ changed.
add_custom_target(site_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/site/assets)
