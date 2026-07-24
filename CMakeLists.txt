cmake_minimum_required(VERSION 3.21)

set(CMAKE_SYSTEM_VERSION 10.0.22621.0 CACHE STRING "Windows SDK version" FORCE)

# 프로젝트 이름 설정
project(RieuxRenderer VERSION 1.0)

# C++17 표준 설정
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 소스 파일 수집
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.h"
    "src/*.hlsl"
    "src/*.hlsli"
)

# 실행 파일 생성 (윈도우 애플리케이션)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# vcpkg 패키지 불러오기
find_package(imgui CONFIG REQUIRED)
find_package(directxtk12 CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# 헤더 포함 경로 설정
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/D3D12"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/Managers"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/System"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Components"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Data"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/Tool"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/World"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Utils"
)

# 라이브러리 연결
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    dxcompiler.lib
    
    imgui::imgui
    Microsoft::DirectXTK12
    Microsoft::DirectXTex
    spdlog::spdlog
    assimp::assimp
)

# 미리 컴파일된 헤더 설정
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/Pch.h")

# HLSL 컴파일 설정
foreach(FILE ${SOURCES})
    if(FILE MATCHES "\\.hlsl$")
        set_source_files_properties(${FILE} PROPERTIES VS_TOOL_OVERRIDE "FXCompile")
        set_source_files_properties(${FILE} PROPERTIES VS_SHADER_MODEL "6.0")
        
        if(FILE MATCHES "VS" OR FILE MATCHES "Vertex")
            set_source_files_properties(${FILE} PROPERTIES VS_SHADER_TYPE "Vertex")
        elseif(FILE MATCHES "PS" OR FILE MATCHES "Pixel")
            set_source_files_properties(${FILE} PROPERTIES VS_SHADER_TYPE "Pixel")
        elseif(FILE MATCHES "CS" OR FILE MATCHES "Compute")
            set_source_files_properties(${FILE} PROPERTIES VS_SHADER_TYPE "Compute")
        elseif(FILE MATCHES "HS" OR FILE MATCHES "Hull")
            set_source_files_properties(${FILE} PROPERTIES VS_SHADER_TYPE "Hull")
        elseif(FILE MATCHES "DS" OR FILE MATCHES "Domain")
            set_source_files_properties(${FILE} PROPERTIES VS_SHADER_TYPE "Domain")
        endif()
    elseif(FILE MATCHES "\\.hlsli$")
        set_source_files_properties(${FILE} PROPERTIES VS_TOOL_OVERRIDE "None")
    endif()
endforeach()

# 빌드 폴더로 에셋 및 셰이더 복사 설정
set(HLSL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Graphics/HLSL")
set(HLSL_OUTPUT_DIR "${CMAKE_BINARY_DIR}/HLSL")
set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${HLSL_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${HLSL_SOURCE_DIR}" "${HLSL_OUTPUT_DIR}"
    
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${ASSETS_OUTPUT_DIR}"
    
    COMMENT "Copying HLSL shaders and Assets to build directory..."
)