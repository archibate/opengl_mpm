cmake_minimum_required(VERSION 3.17)
project(hello_opengl)


file(GLOB PROJECT_SOURCE "*.cpp" "*.h")

include_directories(${PROJECT_NAME} ".")
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE})
target_link_libraries(${PROJECT_NAME} OpenGL)
target_link_libraries(${PROJECT_NAME} GLEW)
target_link_libraries(${PROJECT_NAME} glfw)
