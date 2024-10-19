add_library(Core STATIC)

target_sources(Core 
  PRIVATE
    Application.cpp
    Renderer.cpp
    VertexBuffer.cpp
    IndexBuffer.cpp
    VertexArray.cpp
    Shader.cpp
    Texture.cpp
    VertexBufferLayout.cpp
    Serialization/StreamWriter.cpp
    Serialization/StreamReader.cpp
    Serialization/BufferStream.cpp
    Serialization/FileStream.cpp
)
target_include_directories(Core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(Core PUBLIC glfw)
target_link_libraries(Core PUBLIC OpenGL::GL)
target_link_libraries(Core PUBLIC GLEW::GLEW)
target_link_libraries(Core PUBLIC glm::glm-header-only)
target_link_libraries(Core PUBLIC fmt::fmt-header-only)
target_link_libraries(Core PUBLIC portaudio)

target_link_libraries(Core PUBLIC Vendors)
