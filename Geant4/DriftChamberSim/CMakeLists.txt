cmake_minimum_required(VERSION 3.16)
project(DriftChamberSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#----------------------------------------------------------------------------
# Geant4
#----------------------------------------------------------------------------
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# ROOT
#----------------------------------------------------------------------------
set(ROOT_DIR "/opt/homebrew/Cellar/root/6.38.04_1")
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

message(STATUS "ROOT version: ${ROOT_VERSION}")
message(STATUS "ROOT libraries: ${ROOT_LIBRARIES}")

#----------------------------------------------------------------------------
# Build GenFit2
#----------------------------------------------------------------------------
include(ExternalProject)
set(GENFIT2_INSTALL_DIR ${CMAKE_BINARY_DIR}/genfit2_install)

ExternalProject_Add(
  genfit2_external
  GIT_REPOSITORY https://github.com/GenFit/GenFit.git
  GIT_TAG main
  PREFIX ${CMAKE_BINARY_DIR}/genfit2
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GENFIT2_INSTALL_DIR}
             -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
             -DROOT_DIR=${ROOT_DIR}
             -DEIGEN3_INCLUDE_DIR=/opt/homebrew/include/eigen3
             -DBUILD_TESTING=OFF
             -DBUILD_TESTS=OFF
             -DGENFIT_BUILD_TESTS=OFF
)

#----------------------------------------------------------------------------
# GenFit2 library
#----------------------------------------------------------------------------
set(GENFIT2_LIBRARY ${GENFIT2_INSTALL_DIR}/lib/libgenfit2.2.3.dylib)
message(STATUS "GenFit2 library: ${GENFIT2_LIBRARY}")

#----------------------------------------------------------------------------
# Sources
#----------------------------------------------------------------------------
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/include/*.hh)

list(REMOVE_ITEM SOURCES "${PROJECT_SOURCE_DIR}/src/KalmanFit.cc")
list(REMOVE_ITEM HEADERS "${PROJECT_SOURCE_DIR}/include/KalmanFit.hh")

#----------------------------------------------------------------------------
# ROOT dictionary
#----------------------------------------------------------------------------
root_generate_dictionary(
  G__TrackedParticle
  ${PROJECT_SOURCE_DIR}/include/TrackedParticle.hh
  ${PROJECT_SOURCE_DIR}/include/TrackSegment.hh
  MODULE DriftChamberSim
  LINKDEF ${PROJECT_SOURCE_DIR}/include/LinkDef.hh
)

#----------------------------------------------------------------------------
# Common include directories
#----------------------------------------------------------------------------
set(COMMON_INCLUDES
    ${PROJECT_SOURCE_DIR}/include
    ${GENFIT2_INSTALL_DIR}/include
)

#----------------------------------------------------------------------------
# Common libraries
#----------------------------------------------------------------------------
set(COMMON_LIBRARIES
    ${Geant4_LIBRARIES}
    ${GENFIT2_LIBRARY}   # GenFit2 linked before ROOT on macOS
    ${ROOT_LIBRARIES}
)

#----------------------------------------------------------------------------
# Executable: DriftChamberSim
#----------------------------------------------------------------------------
add_executable(DriftChamberSim DriftChamberSim.cc ${SOURCES} ${HEADERS} G__TrackedParticle.cxx)
target_include_directories(DriftChamberSim PRIVATE ${COMMON_INCLUDES})
target_link_libraries(DriftChamberSim PRIVATE ${COMMON_LIBRARIES})

add_dependencies(DriftChamberSim genfit2_external)

set_target_properties(DriftChamberSim PROPERTIES
  BUILD_RPATH "${GENFIT2_INSTALL_DIR}/lib"
  INSTALL_RPATH "${GENFIT2_INSTALL_DIR}/lib"
)

#----------------------------------------------------------------------------
# Executable: kalman
#----------------------------------------------------------------------------
add_executable(kalman 
  ${PROJECT_SOURCE_DIR}/src/KalmanFit.cc 
  ${PROJECT_SOURCE_DIR}/include/KalmanFit.hh
  ${SOURCES} 
  G__TrackedParticle.cxx
)

target_include_directories(kalman PRIVATE 
  ${COMMON_INCLUDES}
  ${GENFIT2_INSTALL_DIR}/include
)

set_target_properties(kalman PROPERTIES
  BUILD_RPATH "${GENFIT2_INSTALL_DIR}/lib"
  INSTALL_RPATH "${GENFIT2_INSTALL_DIR}/lib"
)

add_dependencies(kalman genfit2_external)
target_link_libraries(kalman PRIVATE ${COMMON_LIBRARIES})

#----------------------------------------------------------------------------
# Shared library
#----------------------------------------------------------------------------
add_library(DriftChamberlib SHARED DriftChamberSim.cc ${SOURCES} ${HEADERS} G__TrackedParticle.cxx)
target_include_directories(DriftChamberlib PRIVATE ${COMMON_INCLUDES})
target_link_libraries(DriftChamberlib PRIVATE ${COMMON_LIBRARIES})

add_dependencies(DriftChamberlib genfit2_external)

set_target_properties(DriftChamberlib PROPERTIES
  BUILD_RPATH "${GENFIT2_INSTALL_DIR}/lib"
  INSTALL_RPATH "${GENFIT2_INSTALL_DIR}/lib"
)

#----------------------------------------------------------------------------
# Copy macros/scripts
#----------------------------------------------------------------------------
set(DRIFTCHAMBER_SCRIPTS
    gui.mac
    init_vis.mac
    vis.mac
)

foreach(script ${DRIFTCHAMBER_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${script}
    ${PROJECT_BINARY_DIR}/${script}
    COPYONLY
  )
endforeach()
