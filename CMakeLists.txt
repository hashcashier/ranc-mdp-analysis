cmake_minimum_required(VERSION 3.2)

# set the project name and version
project(ranc-mdp-analysis VERSION 1.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# search for Storm library
find_package(storm REQUIRED)

# specify source files
set(SOURCE_FILES src/util.h src/util.cpp src/main.cpp src/model.h src/model.cpp src/model_explicit.h src/model_explicit.cpp src/model_explicit_incentive_extended.h src/model_explicit_incentive_extended.cpp src/simulator_incentive_extended.h src/model_explicit_subversion_extended.h src/model_explicit_subversion_extended.cpp src/model_explicit_censorship_extended.h src/model_explicit_censorship_extended.cpp src/simulator.h src/simulator.cpp src/simulator_subversion_extended.h src/simulator_censorship_extended.h src/model_explicit_quality_extended.h src/model_explicit_quality_extended.cpp src/simulator_quality_extended.h src/model_explicit_reachability_extended.h src/model_explicit_reachability_extended.cpp src/model_explicit_censorship_nakamoto.h src/model_explicit_censorship_nakamoto.cpp src/simulator_censorship_nakamoto.h src/model_explicit_subversion_nakamoto.h src/model_explicit_subversion_nakamoto.cpp src/simulator_subversion_nakamoto.h)

# set executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${SOURCE_FILES} ${storm_INCLUDE_DIR} ${storm-parsers_INCLUDE_DIR} ${storm-pars_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE storm storm-parsers storm-pars)

