add_executable(decoder decoder.cpp)

# Include marian's source directory where the headers are located
target_include_directories(decoder PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(decoder PRIVATE llama)