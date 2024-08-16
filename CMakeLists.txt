cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(REA)

add_compile_definitions(SE_DEBUG_LOG)
add_compile_definitions(B2_USER_SETTINGS)

file(DOWNLOAD
        https://raw.githubusercontent.com/juliettef/IconFontCppHeaders/main/IconsFontAwesome6.h
        ${CMAKE_SOURCE_DIR}/external/include/IconsFontAwesome.h
)

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool
add_subdirectory(submodules/FastNoise2)

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
        src/Sprite.cpp
        include/REA/Sprite.hpp
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
        src/System/PixelGridDrawing.cpp
        include/REA/System/PixelGridDrawing.hpp
        src/System/ImGuiManager.cpp
        include/REA/System/ImGuiManager.hpp
        include/REA/Stage.hpp
        ${CMAKE_SOURCE_DIR}/external/include/IconsFontAwesome.h
        include/REA/PixelType.hpp
        src/Math.cpp
        include/REA/Math.hpp
        src/PixelGridBuilder.cpp
        include/REA/PixelGridBuilder.hpp
        include/REA/Context/ImGui.hpp
        include/REA/Component/PixelGridRenderer.hpp
        include/REA/Component/Collider.hpp
        src/PhysicsMaterial.cpp
        include/REA/PhysicsMaterial.hpp
        include/b2_user_settings.h
        src/System/PhysicsDebugRenderer.cpp
        include/REA/System/PhysicsDebugRenderer.hpp
        src/MarchingSquareMesherUtils.cpp
        include/REA/MarchingSquareMesherUtils.hpp
        include/REA/Component/PixelGridRigidBody.hpp
        src/MemoryHeap.cpp
        include/REA/MemoryHeap.hpp
        src/WorldGenerator.cpp
        include/REA/WorldGenerator.hpp
        include/REA/Constants.hpp
        src/System/ReaSystem.cpp
        include/REA/System/ReaSystem.hpp
        src/System/MainMenu.cpp
        include/REA/System/MainMenu.hpp
        include/REA/ImGuiHelper.hpp
        src/ImGuiHelper.cpp
        src/System/PauseMenu.cpp
        include/REA/System/PauseMenu.hpp
        src/System/SandboxController.cpp
        include/REA/System/SandboxController.hpp
        include/REA/Component/SandboxController.hpp
)

target_include_directories(REA PRIVATE include)
target_include_directories(REA PRIVATE external/include)

find_package(SplitEngine CONFIG REQUIRED)
target_link_libraries(REA PRIVATE SplitEngine::SplitEngine)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(REA PRIVATE imgui::imgui)

find_package(box2d CONFIG REQUIRED)
target_link_libraries(REA PRIVATE box2d::box2d)

find_package(CDT CONFIG REQUIRED)
target_link_libraries(REA PRIVATE CDT::CDT)

target_link_libraries(REA PRIVATE FastNoise2)

if(MSVC)
    target_compile_options(REA PRIVATE "/MP")
endif()

add_custom_target(ClearResources ALL
        COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:REA>/res
        COMMENT "Clearing Resources"
)

add_custom_target(CopyResources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:REA>/res
        COMMENT "Copy Resources"
)
add_dependencies(CopyResources ClearResources)

set(SHADER_FILES
        "res/shaders/Sprite/Sprite.vert"
        "res/shaders/Sprite/Sprite.frag"
        "res/shaders/PixelGrid/PixelGrid.vert"
        "res/shaders/PixelGrid/PixelGrid.frag"
        "res/shaders/PhysicsDebug/PhysicsDebug.vert"
        "res/shaders/PhysicsDebug/PhysicsDebug.frag"
        "res/shaders/PixelGridRigidBody/PixelGridRigidBody.comp"
        "res/shaders/PixelGridRigidBodyRemove/PixelGridRigidBodyRemove.comp"
        "res/shaders/PixelGridComputeFalling/PixelGridComputeFalling.comp"
        "res/shaders/PixelGridComputeIdle/PixelGridComputeIdle.comp"
        "res/shaders/PixelGridComputeAccumulate/PixelGridComputeAccumulate.comp"
        "res/shaders/PixelGridComputeParticle/PixelGridComputeParticle.comp"
        "res/shaders/MarchingSquare/MarchingSquare.comp"
        "res/shaders/CCLInitialize/CCLInitialize.comp"
        "res/shaders/CCLColumn/CCLColumn.comp"
        "res/shaders/CCLMerge/CCLMerge.comp"
        "res/shaders/CCLRelabel/CCLRelabel.comp"
        "res/shaders/CCLExtract/CCLExtract.comp"
)

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

add_custom_target(CompileShaders ALL)
add_dependencies(CompileShaders CopyResources)

foreach (SHADER_FILE ${SHADER_FILES})
    set(SPIRV_OUTPUT $<TARGET_FILE_DIR:REA>/${SHADER_FILE}.spv)

    add_custom_command(TARGET CompileShaders PRE_BUILD
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





add_dependencies(REA
        CopyResources
        CompileShaders
)
