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
  # Used from EM_ASM blocks (engine/audio.cpp, engine/net.cpp).
  "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToUTF8,$UTF8ToString,$stringToNewUTF8"
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

# The site folder (build/site): one staging step. Every file is its own
# build rule, so only changed files are copied (and models re-gzipped).
set(SITE ${CMAKE_BINARY_DIR}/site)
file(GLOB_RECURSE WEB_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/web/*)
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/*)
set(SITE_OUTPUTS)
foreach(src ${WEB_FILES})
  file(RELATIVE_PATH rel ${CMAKE_SOURCE_DIR}/web ${src})
  add_custom_command(OUTPUT ${SITE}/${rel} DEPENDS ${src}
                     COMMAND ${CMAKE_COMMAND} -E copy ${src} ${SITE}/${rel} VERBATIM)
  list(APPEND SITE_OUTPUTS ${SITE}/${rel})
endforeach()
foreach(src ${ASSET_FILES})
  file(RELATIVE_PATH rel ${CMAKE_SOURCE_DIR} ${src})
  get_filename_component(dir ${SITE}/${rel} DIRECTORY)
  if(src MATCHES "\\.glb$")
    # Models are served gzipped (about half the size); the engine unpacks
    # them with the browser's DecompressionStream (engine/fetch.cpp). -n
    # keeps the output identical between builds.
    add_custom_command(OUTPUT ${SITE}/${rel}.gz DEPENDS ${src}
                       COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
                       COMMAND sh -c "gzip -9 -n -c \"$0\" > \"$1\"" ${src} ${SITE}/${rel}.gz VERBATIM)
    list(APPEND SITE_OUTPUTS ${SITE}/${rel}.gz)
  else()
    add_custom_command(OUTPUT ${SITE}/${rel} DEPENDS ${src}
                       COMMAND ${CMAKE_COMMAND} -E copy ${src} ${SITE}/${rel} VERBATIM)
    list(APPEND SITE_OUTPUTS ${SITE}/${rel})
  endif()
endforeach()
add_custom_command(OUTPUT ${SITE}/app.js ${SITE}/app.wasm DEPENDS app
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:app>/app.js $<TARGET_FILE_DIR:app>/app.wasm ${SITE}
                   VERBATIM)
list(APPEND SITE_OUTPUTS ${SITE}/app.js ${SITE}/app.wasm)
add_custom_target(site ALL DEPENDS ${SITE_OUTPUTS})
