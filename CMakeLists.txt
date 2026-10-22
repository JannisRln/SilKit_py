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

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

set(PYBIND11_ENABLE_SMART_HOLDER ON CACHE BOOL "" FORCE)

include(ExternalProject)

# ---------------------------------------------------------------------------
# SilKit paths
# ---------------------------------------------------------------------------

set(SILKIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/vendor/sil-kit")
set(SILKIT_BUILD_DIR  "${CMAKE_SOURCE_DIR}/build-silkit")

# ---------------------------------------------------------------------------
# Automatic SilKit build if native library not found
# ---------------------------------------------------------------------------

if (WIN32)
    set(SILKIT_LIBRARY_PATH "${SILKIT_BUILD_DIR}/Release/SilKit.dll")
    set(SILKIT_IMPLIB_PATH  "${SILKIT_BUILD_DIR}/Release/SilKit.lib")
else()
    set(SILKIT_LIBRARY_PATH "${SILKIT_BUILD_DIR}/Release/${CMAKE_SHARED_LIBRARY_PREFIX}SilKit${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
set(SILKIT_REGISTRY_PATH "${SILKIT_BUILD_DIR}/Release/sil-kit-registry${CMAKE_EXECUTABLE_SUFFIX}")
set(SILKIT_SYSTEM_CONTROLLER_PATH "${SILKIT_BUILD_DIR}/Release/sil-kit-system-controller${CMAKE_EXECUTABLE_SUFFIX}")
set(SILKIT_MONITOR_PATH "${SILKIT_BUILD_DIR}/Release/sil-kit-monitor${CMAKE_EXECUTABLE_SUFFIX}")

set(SILKIT_CXX_FLAGS)
if (MSVC)
    list(APPEND SILKIT_CXX_FLAGS "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} /FS")
endif()

# The utilities are copied into the package's bin/ directory.  Keep their
# runtime search path relative to their own location so that they can find the
# duplicate runtime library after the wheel is installed.
if (APPLE)
    set(SILKIT_BUILD_RPATH "@loader_path")
elseif (UNIX)
    set(SILKIT_BUILD_RPATH "$ORIGIN")
endif()

if (NOT EXISTS "${SILKIT_LIBRARY_PATH}"
    OR NOT EXISTS "${SILKIT_REGISTRY_PATH}"
    OR NOT EXISTS "${SILKIT_SYSTEM_CONTROLLER_PATH}"
    OR NOT EXISTS "${SILKIT_MONITOR_PATH}")
    message(STATUS "SIL Kit runtime or utilities not found. Automatically building SIL Kit...")

    ExternalProject_Add(SilKitExternal
        SOURCE_DIR        ${SILKIT_SOURCE_DIR}
        BINARY_DIR        ${SILKIT_BUILD_DIR}
        CONFIGURE_COMMAND ${CMAKE_COMMAND}
            ${SILKIT_CXX_FLAGS}
            -DSILKIT_BUILD_TESTS=OFF
            -DSILKIT_BUILD_EXAMPLES=OFF
            -DSILKIT_BUILD_DEMOS=OFF
            -DSILKIT_INSTALL_SOURCE=OFF
            -DSILKIT_BUILD_UTILITIES=ON #Needed for unittest 
            -DCMAKE_BUILD_RPATH=${SILKIT_BUILD_RPATH}
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_BUILD_TYPE=Release
            -S ${SILKIT_SOURCE_DIR}
            -B ${SILKIT_BUILD_DIR}
        BUILD_COMMAND     ${CMAKE_COMMAND} --build ${SILKIT_BUILD_DIR} --config Release
        INSTALL_COMMAND   ""  # do not install system-wide
        BUILD_BYPRODUCTS  ${SILKIT_LIBRARY_PATH} ${SILKIT_IMPLIB_PATH}
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
    IMPORTED_LOCATION     "${SILKIT_LIBRARY_PATH}"
    INTERFACE_INCLUDE_DIRECTORIES "${SILKIT_SOURCE_DIR}/SilKit/include"
)
if (WIN32)
    set_target_properties(SilKit PROPERTIES IMPORTED_IMPLIB "${SILKIT_IMPLIB_PATH}")
endif()

# Ensure _core waits for SilKitExternal if necessary
add_dependencies(SilKit SilKitExternal)

# ---------------------------------------------------------------------------
# Build Python extension module
# ---------------------------------------------------------------------------

pybind11_add_module(_core MODULE src/main.cpp)
target_link_libraries(_core PRIVATE SilKit)

target_compile_definitions(_core PRIVATE VERSION_INFO=${PROJECT_VERSION})

if (APPLE)
    set_target_properties(_core PROPERTIES INSTALL_RPATH "@loader_path")
elseif (UNIX)
    set_target_properties(_core PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()

# ---------------------------------------------------------------------------
# Install into the Python wheel
# ---------------------------------------------------------------------------

# Install the Python extension
install(TARGETS _core DESTINATION SilKit_py)

# Install the runtime beside the extension module.  The utilities receive a
# second copy in bin/, because native loaders search the executable directory
# before the parent Python package directory on both Windows and Linux.
install(FILES "${SILKIT_LIBRARY_PATH}" DESTINATION SilKit_py)
install(FILES "${SILKIT_LIBRARY_PATH}" DESTINATION SilKit_py/bin)

# Ship the utilities that make a Python-only installation self-contained.
# CMAKE_EXECUTABLE_SUFFIX is .exe on Windows and empty on Linux.
foreach(SILKIT_TOOL sil-kit-registry sil-kit-system-controller sil-kit-monitor)
    install(
        PROGRAMS "${SILKIT_BUILD_DIR}/Release/${SILKIT_TOOL}${CMAKE_EXECUTABLE_SUFFIX}"
        DESTINATION SilKit_py/bin
    )
endforeach()

# Preserve SIL Kit's and its third-party notices in the binary distribution.
install(FILES
    "${SILKIT_SOURCE_DIR}/LICENSE"
    "${SILKIT_SOURCE_DIR}/ThirdParty/LICENSES.rst"
    DESTINATION SilKit_py/licenses
)
