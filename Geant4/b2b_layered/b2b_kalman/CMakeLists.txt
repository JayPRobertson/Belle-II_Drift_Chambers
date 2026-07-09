cmake_minimum_required(VERSION 3.16)

project(B2b)

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

set(GENFIT2_INSTALL_DIR
    ${CMAKE_BINARY_DIR}/genfit2_install
)

ExternalProject_Add(
    genfit2_external

    GIT_REPOSITORY https://github.com/GenFit/GenFit.git
    GIT_TAG main

    PREFIX ${CMAKE_BINARY_DIR}/genfit2

    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${GENFIT2_INSTALL_DIR}
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
set(GENFIT2_LIBRARY
    ${GENFIT2_INSTALL_DIR}/lib/libgenfit2.2.3.dylib
)

message(STATUS "GenFit2 library: ${GENFIT2_LIBRARY}")


#----------------------------------------------------------------------------
# Sources
#----------------------------------------------------------------------------
file(GLOB SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cc
)

file(GLOB HEADERS
    ${PROJECT_SOURCE_DIR}/include/*.hh
)


#----------------------------------------------------------------------------
# ROOT dictionary
#----------------------------------------------------------------------------
root_generate_dictionary(
    G__TrackedParticle

    ${PROJECT_SOURCE_DIR}/include/TrackedParticle.hh
    ${PROJECT_SOURCE_DIR}/include/TrackSegment.hh

    MODULE B2b

    LINKDEF
    ${PROJECT_SOURCE_DIR}/include/LinkDef.hh
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

    # GenFit2 must appear before ROOT on macOS
    ${GENFIT2_LIBRARY}

    ${ROOT_LIBRARIES}
)


#----------------------------------------------------------------------------
# Executable
#----------------------------------------------------------------------------
add_executable(exampleB2b

    exampleB2b.cc

    ${SOURCES}
    ${HEADERS}

    G__TrackedParticle.cxx
)


target_include_directories(exampleB2b PRIVATE
    ${COMMON_INCLUDES}
)


# Runtime library search path for macOS
set_target_properties(exampleB2b PROPERTIES

    BUILD_RPATH
        "${GENFIT2_INSTALL_DIR}/lib"

    INSTALL_RPATH
        "${GENFIT2_INSTALL_DIR}/lib"
)


add_dependencies(exampleB2b
    genfit2_external
)


target_link_libraries(exampleB2b PRIVATE
    ${COMMON_LIBRARIES}
)



#----------------------------------------------------------------------------
# Shared library
#----------------------------------------------------------------------------
add_library(exampleB2blib SHARED

    exampleB2b.cc

    ${SOURCES}
    ${HEADERS}

    G__TrackedParticle.cxx
)


target_include_directories(exampleB2blib PRIVATE
    ${COMMON_INCLUDES}
)


set_target_properties(exampleB2blib PROPERTIES

    BUILD_RPATH
        "${GENFIT2_INSTALL_DIR}/lib"

    INSTALL_RPATH
        "${GENFIT2_INSTALL_DIR}/lib"
)


add_dependencies(exampleB2blib
    genfit2_external
)


target_link_libraries(exampleB2blib PRIVATE
    ${COMMON_LIBRARIES}
)



#----------------------------------------------------------------------------
# Copy macros/scripts
#----------------------------------------------------------------------------
set(EXAMPLEB2B_SCRIPTS

    exampleB2b.out
    exampleB2.in
    gui.mac
    init_vis.mac
    run1.mac
    run2.mac
    vis.mac
)


foreach(script ${EXAMPLEB2B_SCRIPTS})

    configure_file(
        ${PROJECT_SOURCE_DIR}/${script}
        ${PROJECT_BINARY_DIR}/${script}
        COPYONLY
    )

endforeach()