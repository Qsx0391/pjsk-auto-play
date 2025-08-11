cmake_minimum_required(VERSION 3.16)
project(pjsk-auto-play LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1 /W3")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /W3")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1 /DNDEBUG /W3")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/MD /O1 /Ob1 /DNDEBUG /W3")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -DNDEBUG -Wall")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG -Wall")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(OpenCV REQUIRED)
find_package(spdlog CONFIG REQUIRED)

file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB UI_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/form/*.ui"
)
file(GLOB QRC_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/resource/*.qrc"
)

add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${HEADERS}
        ${UI_FILES}
        ${QRC_FILES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/common
        ${CMAKE_CURRENT_SOURCE_DIR}/include/image
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mumu
        ${CMAKE_CURRENT_SOURCE_DIR}/include/player
        ${CMAKE_CURRENT_SOURCE_DIR}/include/screen
        ${CMAKE_CURRENT_SOURCE_DIR}/include/touch
        ${CMAKE_CURRENT_SOURCE_DIR}/include/window
        ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Qt6::Widgets
        ${OpenCV_LIBS}
        spdlog::spdlog_header_only
)

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${Qt6_DIR}/../../../bin" "${Qt6_DIR}/../../../../bin")
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-compiler-runtime --no-translations --no-system-d3d-compiler $<TARGET_FILE:${PROJECT_NAME}>
                WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif ()

    get_filename_component(OPENCV_ROOT_DIR ${OpenCV_DIR} DIRECTORY)
    get_filename_component(OPENCV_ROOT_DIR ${OPENCV_ROOT_DIR} DIRECTORY)
    set(OPENCV_BIN_DIR "${OPENCV_ROOT_DIR}/bin")
    if (EXISTS ${OPENCV_BIN_DIR})
        file(GLOB OPENCV_DLLS "${OPENCV_BIN_DIR}/opencv_world*.dll")
        foreach (OPENCV_DLL ${OPENCV_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OPENCV_DLL}
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endforeach ()
    endif ()
endif ()

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
)

include(InstallRequiredSystemLibraries)
if (WIN32)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    install(PROGRAMS $<TARGET_FILE:${PROJECT_NAME}> DESTINATION bin)
endif ()