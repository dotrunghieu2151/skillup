add_executable(skillup App.cpp)

target_include_directories(skillup PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(skillup PRIVATE Core)

add_subdirectory(Modules/TaskManagement)

target_link_libraries(skillup PRIVATE TaskManagement)

add_subdirectory(Modules/Transcribe)

target_link_libraries(skillup PRIVATE Transcribe)


# Copy assets only when they change
set (assetSource "${CMAKE_SOURCE_DIR}/Assets")
file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS "${assetSource}/*")

# Create a stamp file to track when assets were last copied
set(ASSETS_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets_copied.stamp")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M:%S")

# Add custom command with OUTPUT (avoids circular dependency)
add_custom_command(
    OUTPUT ${ASSETS_STAMP_FILE}
    DEPENDS ${ASSET_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${assetSource} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets
    COMMAND ${CMAKE_COMMAND} -E touch ${ASSETS_STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E echo "Assets copied: Date: ${BUILD_DATE}" > ${ASSETS_STAMP_FILE}
    COMMENT "Copying updated assets: ${assetSource} to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets"
)

# Create a custom target that depends on the stamp file
add_custom_target(copy_assets DEPENDS ${ASSETS_STAMP_FILE})

# Make skillup depend on the asset copying
add_dependencies(skillup copy_assets)