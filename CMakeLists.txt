cmake_minimum_required(VERSION 3.16)
project(INDITestClient VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# Set up pkg-config paths for INDI and CFITSIO
set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)

# Find INDI using pkg-config
pkg_check_modules(INDI REQUIRED libindi)

# Find CFITSIO using pkg-config from /opt/homebrew/lib/pkgconfig/cfitsio.pc
pkg_check_modules(CFITSIO REQUIRED cfitsio)

# Find StellarSolver
# First try pkg-config
pkg_check_modules(STELLARSOLVER stellarsolver)

# If not found via pkg-config, try manual find
if(NOT STELLARSOLVER_FOUND)
    # Try to find StellarSolver in common locations
    find_path(STELLARSOLVER_INCLUDE_DIRS
        NAMES stellarsolver.h
        PATHS /usr/local/include/stellarsolver
              /opt/homebrew/include/stellarsolver
              /usr/include/stellarsolver
    )
    
    find_library(STELLARSOLVER_LIBRARIES
        NAMES stellarsolver
        PATHS /usr/local/lib
              /opt/homebrew/lib
              /usr/lib
    )
    
    if(STELLARSOLVER_INCLUDE_DIRS AND STELLARSOLVER_LIBRARIES)
        set(STELLARSOLVER_FOUND TRUE)
    else()
        message(FATAL_ERROR "StellarSolver not found. Please install StellarSolver or specify paths manually.")
    endif()
endif()

# Output information about found packages
message(STATUS "CFITSIO Version: ${CFITSIO_VERSION}")
message(STATUS "CFITSIO Include Dirs: ${CFITSIO_INCLUDE_DIRS}")
message(STATUS "CFITSIO Libraries: ${CFITSIO_LIBRARIES}")

message(STATUS "INDI Include Dirs: ${INDI_INCLUDE_DIRS}")
message(STATUS "INDI Libraries: ${INDI_LIBRARIES}")

message(STATUS "StellarSolver Include Dirs: ${STELLARSOLVER_INCLUDE_DIRS}")
message(STATUS "StellarSolver Libraries: ${STELLARSOLVER_LIBRARIES}")

# Include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${INDI_INCLUDE_DIRS}
  ${CFITSIO_INCLUDE_DIRS}
  ${STELLARSOLVER_INCLUDE_DIRS}
)

# Add library directories
link_directories(
  ${INDI_LIBRARY_DIRS}
  ${CFITSIO_LIBRARY_DIRS}
  ${STELLARSOLVER_LIBRARY_DIRS}
)

# Source files
set(SOURCES
  main.cpp
  INDIClient.cpp
  MainWindow.cpp
  CameraPanel.cpp
  MountPanel.cpp
  PlateSolverPanel.cpp
  MountModelPanel.cpp
  ImagePanel.cpp
  StellarSolverInterface.cpp
)

# Header files
set(HEADERS
  INDITestClient.h
  StellarSolverInterface.h
)

# Create executable
add_executable(indi_test_client ${SOURCES} ${HEADERS})

# Add Qt MOC generation
set_target_properties(indi_test_client PROPERTIES AUTOMOC TRUE)

# Link libraries
target_link_libraries(indi_test_client PRIVATE
  Qt6::Core
  Qt6::Widgets
  ${INDI_LIBRARIES}
  ${CFITSIO_LIBRARIES}
  ${STELLARSOLVER_LIBRARIES}
)

# Add compiler flags from pkg-config
target_compile_options(indi_test_client PRIVATE 
  ${INDI_CFLAGS}
  ${CFITSIO_CFLAGS}
  ${STELLARSOLVER_CFLAGS}
)

# Add linker flags from pkg-config
target_link_options(indi_test_client PRIVATE 
  ${INDI_LDFLAGS}
  ${CFITSIO_LDFLAGS}
  ${STELLARSOLVER_LDFLAGS}
)

# Install targets
install(TARGETS indi_test_client DESTINATION bin)

# Create package if requested
option(MAKE_PACKAGE "Create package" OFF)
if(MAKE_PACKAGE)
  include(CPack)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_NAME "indi-test-client")
  set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
  set(CPACK_PACKAGE_CONTACT "your@email.com")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "INDI Test Client")
  set(CPACK_PACKAGE_DESCRIPTION "A Qt-based INDI client for testing mount/camera functionality with plate solving and mount modeling")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libindi1, libqt6core6, libqt6widgets6, libcfitsio10, stellarsolver")
endif()