cmake_minimum_required(VERSION 3.0.0)

# set(VCPKG_LIBRARY_LINKAGE static)
# set(VCPKG_CRT_LINKAGE static)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
  # add_compile_options(-Ofast)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
else ()
  set(CMAKE_CXX_FLAGS_RELEASE "-Ox")
endif()

project(skillup VERSION 0.1.0 LANGUAGES CXX)


# Packagegs

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)


# Include dir
add_subdirectory(Vendors)
add_subdirectory(Core)
add_subdirectory(App)
add_subdirectory(Tests)

include(CTest)
enable_testing()
