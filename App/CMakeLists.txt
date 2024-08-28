add_executable(skillup App.cpp)

target_include_directories(skillup PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(skillup PRIVATE Core)
target_link_libraries(skillup PRIVATE Test)

# sym link assets
set (assetSource "${CMAKE_SOURCE_DIR}/Assets")

add_custom_command(
                  TARGET skillup POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:skillup>/Assets
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${assetSource} $<TARGET_FILE_DIR:skillup>/Assets
                   COMMENT "copying Assets: ${assetSource} to $<TARGET_FILE_DIR:skillup>/Assets"
)