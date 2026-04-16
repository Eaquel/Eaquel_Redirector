cmake_minimum_required(VERSION 3.22.1)
project(lsplt LANGUAGES CXX)

find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

set(SOURCES lsplt.cpp)

option(LSPLT_BUILD_SHARED "Build lsplt as shared library" ON)

if(LSPLT_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    target_include_directories(${PROJECT_NAME} PUBLIC include)
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
    target_compile_options(${PROJECT_NAME} PRIVATE -flto)
    target_link_options(${PROJECT_NAME} PRIVATE -flto)
    target_link_libraries(${PROJECT_NAME} PUBLIC log)

    if(NOT DEFINED DEBUG_SYMBOLS_PATH)
        set(DEBUG_SYMBOLS_PATH ${CMAKE_BINARY_DIR}/symbols)
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEBUG_SYMBOLS_PATH}/${ANDROID_ABI}
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${PROJECT_NAME}>
                ${DEBUG_SYMBOLS_PATH}/${ANDROID_ABI}/${PROJECT_NAME}
        COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()

add_library(${PROJECT_NAME}_static STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME}_static PUBLIC include)
target_compile_features(${PROJECT_NAME}_static PUBLIC cxx_std_23)
target_link_libraries(${PROJECT_NAME}_static PUBLIC log)
