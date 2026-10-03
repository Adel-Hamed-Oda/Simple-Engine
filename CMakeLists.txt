set(APPLICATION_NAME "PvJ")

cmake_minimum_required(VERSION 3.10)
project(${APPLICATION_NAME})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(engine)

find_package(OpenGL REQUIRED)

add_executable(${APPLICATION_NAME}
    src/Application.cpp
)

target_link_libraries(${APPLICATION_NAME} PRIVATE Engine)

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET ${APPLICATION_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:${APPLICATION_NAME}>/assets"
        COMMENT "Copying assets folder to output directory..."
    )
endif()