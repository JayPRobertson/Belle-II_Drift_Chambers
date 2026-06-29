#----------------------------------------------------------------------------
# Setup the project
#
cmake_minimum_required(VERSION 3.16...3.27)
project(B2b)

#----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# See the documentation for a guide on how to enable/disable specific components
#
find_package(Geant4 REQUIRED ui_all vis_all)

# Find ROOT with all required core, data storage, and math components
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree MathCore MathMore GenVector)
include(${ROOT_USE_FILE}) # Automatically sets up ROOT include paths and macros

# Include header files from the include directory in compilation
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

#----------------------------------------------------------------------------
# Generate ROOT Dictionaries for custom classes using ClassDef
#
root_generate_dictionary(G__TrackedParticle 
    TrackedParticle.hh 
    TrackSegment.hh 
    MODULE B2b
)

#----------------------------------------------------------------------------
# Add the executable, including the newly generated dictionary source code
#
add_executable(exampleB2b exampleB2b.cc ${sources} ${headers} G__TrackedParticle.cxx)
target_include_directories(exampleB2b PRIVATE include)

# Link both Geant4 and ROOT libraries explicitly to the target executable
target_link_libraries(exampleB2b PRIVATE 
    ${Geant4_LIBRARIES} 
    ROOT::Core 
    ROOT::RIO 
    ROOT::Tree 
    ROOT::MathCore
)

add_library(exampleB2blib SHARED exampleB2b.cc ${sources} ${headers} G__TrackedParticle.cxx)
target_include_directories(exampleB2blib PRIVATE include)

# Link both Geant4 and ROOT libraries explicitly to the target executable
target_link_libraries(exampleB2blib PRIVATE 
    ${Geant4_LIBRARIES} 
    ROOT::Core 
    ROOT::RIO 
    ROOT::Tree 
    ROOT::MathCore
)

#----------------------------------------------------------------------------
# Copy all scripts to the build directory
#
set(EXAMPLEB2B_SCRIPTS
  exampleB2b.out
  exampleB2.in
  gui.mac
  init_vis.mac
  run1.mac
  run2.mac
  vis.mac
)

foreach(_script ${EXAMPLEB2B_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
  )
endforeach()

