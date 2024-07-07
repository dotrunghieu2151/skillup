add_library(Test STATIC)

target_sources(Test
  PRIVATE
    Test.cpp
)
target_include_directories(Test PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(Test PUBLIC Core)