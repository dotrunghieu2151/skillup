add_executable(skillup App.cpp)

target_include_directories(skillup PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(skillup PRIVATE Core)

add_subdirectory(Modules/TaskManagement)

target_link_libraries(skillup PRIVATE TaskManagement)

add_subdirectory(Modules/Transcribe)

target_link_libraries(skillup PRIVATE Transcribe)


# sym link assets
set (assetSource "${CMAKE_SOURCE_DIR}/Assets")

add_custom_command(
                  TARGET skillup POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:skillup>/Assets
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${assetSource} $<TARGET_FILE_DIR:skillup>/Assets
                   COMMENT "copying Assets: ${assetSource} to $<TARGET_FILE_DIR:skillup>/Assets"
)

# Copy Whisper models
set (modelsSource "${CMAKE_SOURCE_DIR}/models")

add_custom_command(
                  TARGET skillup POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:skillup>/models
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${modelsSource} $<TARGET_FILE_DIR:skillup>/models
                   COMMENT "copying Whisper models: ${modelsSource} to $<TARGET_FILE_DIR:skillup>/models"
)

# Copy required DLLs for whisper.cpp and dependencies
add_custom_command(
                  TARGET skillup POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:whisper> $<TARGET_FILE_DIR:skillup>
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ggml> $<TARGET_FILE_DIR:skillup>
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ggml-base> $<TARGET_FILE_DIR:skillup>
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ggml-cpu> $<TARGET_FILE_DIR:skillup>
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:ggml-cuda> $<TARGET_FILE_DIR:skillup>
                   COMMENT "copying whisper.cpp DLLs to executable directory"
)