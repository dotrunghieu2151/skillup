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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

project(skillup VERSION 0.1.0 LANGUAGES CXX)

option (BUILD_TESTS "Override building of tests" ON)
option (MEMORY_DEBUG "Override enabling memory debug" ON)

if (MEMORY_DEBUG)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

# Packagegs

find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)


# Include dir
include_directories(${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(Vendors)
add_subdirectory(Core)
add_subdirectory(App)

if (BUILD_TESTS)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  # install external package from git START
  include(FetchContent) 
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  FetchContent_MakeAvailable(googletest)
  include(googletest)
  
  add_subdirectory(Tests)
  enable_testing()
endif()