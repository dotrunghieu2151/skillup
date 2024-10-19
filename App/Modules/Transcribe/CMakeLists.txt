add_library(Transcribe INTERFACE)

target_include_directories(Transcribe INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(Transcribe INTERFACE Core)