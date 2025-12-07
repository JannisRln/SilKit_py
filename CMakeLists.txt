# Require CMake 3.15+ (matching scikit-build-core)
cmake_minimum_required(VERSION 3.15...4.0)

project(
  ${SKBUILD_PROJECT_NAME}
  VERSION ${SKBUILD_PROJECT_VERSION}
  LANGUAGES CXX)

cmake_policy(VERSION 3.15)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)

set(PYBIND11_ENABLE_SMART_HOLDER ON CACHE BOOL "" FORCE)

include(ExternalProject)

# ---------------------------------------------------------------------------
# SilKit paths
# ---------------------------------------------------------------------------

set(SILKIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/vendor/sil-kit")
set(SILKIT_BUILD_DIR  "${CMAKE_SOURCE_DIR}/build-silkit")

# ---------------------------------------------------------------------------
# Automatic SilKit build if DLL not found
# ---------------------------------------------------------------------------

set(SILKIT_DLL_PATH "${SILKIT_BUILD_DIR}/Release/SilKit.dll")

if (NOT EXISTS "${SILKIT_DLL_PATH}")
    message(STATUS "SilKit not found. Automatically building SilKit...")

    ExternalProject_Add(SilKitExternal
        SOURCE_DIR        ${SILKIT_SOURCE_DIR}
        BINARY_DIR        ${SILKIT_BUILD_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND}
            -DSILKIT_BUILD_TESTS=OFF
            -DSILKIT_BUILD_EXAMPLES=OFF
            -DSILKIT_BUILD_DEMOS=OFF
            -DSILKIT_INSTALL_SOURCE=OFF
            -DSILKIT_BUILD_UTILITIES=ON #Needed for unittest 
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_BUILD_TYPE=Release
            -S ${SILKIT_SOURCE_DIR}
            -B ${SILKIT_BUILD_DIR}
        BUILD_COMMAND     ${CMAKE_COMMAND} --build ${SILKIT_BUILD_DIR} --config Release
        INSTALL_COMMAND   ""  # do not install system-wide
    )
else()
    message(STATUS "SilKit build found. Reusing existing build.")
    add_custom_target(SilKitExternal)
endif()

# ---------------------------------------------------------------------------
# Import SilKit from external build
# ---------------------------------------------------------------------------

add_library(SilKit SHARED IMPORTED)

set_target_properties(SilKit PROPERTIES
    IMPORTED_LOCATION     "${SILKIT_BUILD_DIR}/Release/SilKit.dll"
    IMPORTED_IMPLIB       "${SILKIT_BUILD_DIR}/Release/SilKit.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SILKIT_SOURCE_DIR}/SilKit/include"
)

# Ensure _core waits for SilKitExternal if necessary
add_dependencies(SilKit SilKitExternal)

# ---------------------------------------------------------------------------
# Build Python extension module
# ---------------------------------------------------------------------------

pybind11_add_module(_core MODULE src/main.cpp)
target_link_libraries(_core PRIVATE SilKit)

target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})

# ---------------------------------------------------------------------------
# Install into the Python wheel
# ---------------------------------------------------------------------------

# Install the Python extension
install(TARGETS _core DESTINATION SilKit_py)

# Install SilKit runtime DLLs (the only files needed at runtime)
install(
    DIRECTORY "${SILKIT_BUILD_DIR}/Release/"
    DESTINATION SilKit_py
    FILES_MATCHING PATTERN "*.dll"
)
