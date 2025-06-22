add_library(Vendors STATIC)

target_sources(Vendors 
  PRIVATE
    stbImage/stb_image.cpp
    imguistdlib/imgui_stdlib.cpp
    imguiPlot/imgui_plot.cpp
)


# Add whisper.cpp with CUDA support
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "whisper: build tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "whisper: build examples" FORCE)
set(GGML_CUDA ON CACHE BOOL "ggml: use CUDA" FORCE)

add_subdirectory(whisper.cpp)

add_subdirectory(llama-cpp)
target_compile_options(llama PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->")

target_link_libraries(Vendors PUBLIC imgui::imgui)
target_include_directories(Vendors PUBLIC ${CMAKE_CURRENT_LIST_DIR})