set(VULKAN_SDK_PATH  C:/VulkanSDK/1.3.216.0)

cmake_minimum_required(VERSION 3.24)

set(NAME vk_shader_stash)
project(${NAME} VERSION 0.1.0)

message(STATUS "using ${CMAKE_GENERATOR}")

if (DEFINED VULKAN_SDK_PATH)
    set(VULKAN_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    set(VULKAN_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    set(VULKAN_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_compile_options(-Wdefaulted-function-deleted)

target_include_directories(${PROJECT_NAME} PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${VULKAN_INCLUDE_DIRS})

target_link_directories(${PROJECT_NAME} PUBLIC
        ${VULKAN_LIBRARIES})

target_link_libraries(${PROJECT_NAME} vulkan-1)
