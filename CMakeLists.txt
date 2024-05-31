cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(REA)

add_compile_definitions(SE_DEBUG_LOG)

file(DOWNLOAD
        https://raw.githubusercontent.com/juliettef/IconFontCppHeaders/main/IconsFontAwesome6.h
        ${CMAKE_SOURCE_DIR}/external/include/IconsFontAwesome.h
)

add_executable(
        REA
        main.cpp
        src/System/SpriteRenderer.cpp
        include/REA/System/SpriteRenderer.hpp
        include/REA/Component/Transform.hpp
        include/REA/Component/SpriteRenderer.hpp
        include/REA/Component/Player.hpp
        include/REA/Component/Camera.hpp
        src/System/Camera.cpp
        include/REA/System/Camera.hpp
        src/System/PlayerController.cpp
        include/REA/System/PlayerController.hpp
        include/REA/Component/Physics.hpp
        src/System/Physics.cpp
        include/REA/System/Physics.hpp
        include/REA/Component/AudioSource.hpp
        src/System/AudioSourcePlayer.cpp
        include/REA/System/AudioSourcePlayer.hpp
        include/REA/Assets.hpp
        src/SpriteTexture.cpp
        include/REA/SpriteTexture.hpp
        src/System/Debug.cpp
        include/REA/System/Debug.hpp
        include/REA/System/PixelGridRenderer.hpp
        src/System/PixelGridRenderer.cpp
        src/System/RenderingPreparation.cpp
        include/REA/System/RenderingPreparation.hpp
        include/REA/Component/PixelGrid.hpp
        src/System/PixelGridSimulation.cpp
        include/REA/System/PixelGridSimulation.hpp
        include/REA/Pixel.hpp
        src/System/GameOfLifeSimulation.cpp
        include/REA/System/GameOfLifeSimulation.hpp
        src/System/PixelGridDrawing.cpp
        include/REA/System/PixelGridDrawing.hpp
        src/System/ImGuiManager.cpp
        include/REA/System/ImGuiManager.hpp
        include/REA/Stage.hpp
        ${CMAKE_SOURCE_DIR}/external/include/IconsFontAwesome.h
        include/REA/PixelType.hpp
        src/Color.cpp
        include/REA/Color.hpp
        src/Math.cpp
        include/REA/Math.hpp
        src/PixelGridBuilder.cpp
        include/REA/PixelGridBuilder.hpp
)

target_include_directories(REA PRIVATE include)
target_include_directories(REA PRIVATE external/include)

find_package(SplitEngine CONFIG REQUIRED)
target_link_libraries(REA PRIVATE SplitEngine::SplitEngine)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(REA PRIVATE imgui::imgui)

add_custom_command(TARGET REA POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:REA>/res)

add_custom_target(CopyResources ALL DEPENDS ${CMAKE_SOURCE_DIR}/res)

set(SHADER_FILES
        "res/shaders/debug/Debug.vert"
        "res/shaders/debug/Debug.frag"
        "res/shaders/PixelGrid/PixelGrid.vert"
        "res/shaders/PixelGrid/PixelGrid.frag"
        "res/shaders/PixelGridCompute/PixelGridCompute.comp"
        "res/shaders/PixelGridComputeIdle/PixelGridComputeIdle.comp"
        "res/shaders/PixelGridComputeFlow/PixelGridComputeFlow.comp"
)

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

foreach (SHADER_FILE ${SHADER_FILES})
    set(SPIRV_OUTPUT $<TARGET_FILE_DIR:REA>/${SHADER_FILE}.spv)

    add_custom_command(TARGET REA POST_BUILD
            COMMAND ${glslc_executable}
            -o ${SPIRV_OUTPUT}
            -g # Uncomment to enable debug infos
            $<TARGET_FILE_DIR:REA>/${SHADER_FILE}
            DEPENDS ${CMAKE_SOURCE_DIR}/${SHADER_FILE}
            RESULT_VARIABLE glslang_result
            OUTPUT_VARIABLE glslang_output
            ERROR_VARIABLE glslang_error
            COMMENT "Compiling ${SHADER_FILE} to SPIR-V")

    if (NOT ${glslang_result} EQUAL 0)
        message(FATAL_ERROR "Error compiling ${SHADER_NAME}: ${glslang_output}\n${glslang_error}")
    endif ()

    list(APPEND SPIRV_OUTPUTS ${SPIRV_OUTPUT})
endforeach ()

add_custom_target(CompileShaders ALL DEPENDS ${SPIRV_OUTPUTS})
