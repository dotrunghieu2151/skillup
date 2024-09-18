add_library(TaskManagement INTERFACE)

target_include_directories(TaskManagement INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(TaskManagement INTERFACE Core)