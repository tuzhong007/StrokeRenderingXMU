cmake_minimum_required(VERSION 3.10)
project(GPUStrokeRenderingQuadraticApprox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_LIBRARIES_DIR ${PROJECT_SOURCE_DIR}/Libraries)

# Collect all .cpp files in src
file(GLOB SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.c
)

file(GLOB SRC_LIBRARIES
    ${PROJECT_LIBRARIES_DIR}/src/*.cpp
    ${PROJECT_LIBRARIES_DIR}/src/*.c
)

file(GLOB SHADER_FILES "${PROJECT_SOURCE_DIR}/shaders/*")
file(GLOB HEARDER_FILES "${PROJECT_SOURCE_DIR}/include/*")

# Executable
add_executable(${PROJECT_NAME} ${SRC_FILES} ${SRC_LIBRARIES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_INCLUDE_DIR}
    ${PROJECT_LIBRARIES_DIR}/include
)

# Link directories and libraries
target_link_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_LIBRARIES_DIR}/lib
)

target_link_libraries(${PROJECT_NAME}
    glfw3
    opengl32
)

# organize files in IDEs
target_sources(${PROJECT_NAME} PRIVATE ${SHADER_FILES} ${HEARDER_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
source_group("Headers" FILES ${HEARDER_FILES})
source_group("Source Files" FILES ${SRC_FILES} ${SRC_LIBRARIES})
