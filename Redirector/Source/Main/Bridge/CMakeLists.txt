cmake_minimum_required(VERSION 4.0.0)
project(Redirector LANGUAGES CXX)


find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

# Kaynak dosya: Redirector.cpp (lsplt.cpp değil)
set(SOURCES Redirector.cpp)

option(REDIRECTOR_BUILD_SHARED "Build Redirector as shared library" ON)

if(REDIRECTOR_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    # Prefab header dizini: Bridge/ (include/ değil)
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
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

# Static varyant
add_library(${PROJECT_NAME}_static STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${PROJECT_NAME}_static PUBLIC cxx_std_23)
target_link_libraries(${PROJECT_NAME}_static PUBLIC log)
