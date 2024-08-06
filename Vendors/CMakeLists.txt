add_library(Vendors STATIC)

target_sources(Vendors 
  PRIVATE
    stbImage/stb_image.cpp
    imguistdlib/imgui_stdlib.cpp
)

target_link_libraries(Vendors PUBLIC imgui::imgui)
target_include_directories(Vendors PUBLIC ${CMAKE_CURRENT_LIST_DIR})