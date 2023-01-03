cmake_minimum_required(VERSION 3.21)
project(panda-symmetries)

set(CMAKE_CXX_STANDARD 14)

add_executable(panda-symmetries main.cpp)

# add the directory:
add_subdirectory(libs/panda)
# link the library:
target_link_libraries(panda-symmetries PUBLIC polypanda)  # 'polypanda' is defined in panda/CMakeLists.txt, in the 'add_library' line
# add the panda src dir to our search path for #include *.h files.
target_include_directories(panda-symmetries PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/libs/panda/src")