cmake_minimum_required(VERSION 3.16)

project(ec_test)

# Add header file directory to the search path
include_directories(include)

# Add source files to source directory
aux_source_directory(src/client CLIENT_SOURCES)
aux_source_directory(src/storagenodes storagenodes_SOURCES)
aux_source_directory(src/pswitch PSWITCH_SOURCES)
aux_source_directory(src/share_func SHARE_FUNC_SOURCES)
# aux_source_directory(test TEST_SOURCES)

# Add source directory to executable files 
add_executable(client ${SHARE_FUNC_SOURCES} ${CLIENT_SOURCES})
# add_executable(test ${SHARE_FUNC_SOURCES} ${CLIENT_SOURCES} ${TEST_SOURCES})
add_executable(storagenodes ${SHARE_FUNC_SOURCES} ${storagenodes_SOURCES})
add_executable(pswitch ${SHARE_FUNC_SOURCES} ${PSWITCH_SOURCES})

# Set the output directory for executables
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY build)
# set_target_properties(test PROPERTIES RUNTIME_OUTPUT_DIRECTORY build)
set_target_properties(storagenodes PROPERTIES RUNTIME_OUTPUT_DIRECTORY build)
set_target_properties(pswitch PROPERTIES RUNTIME_OUTPUT_DIRECTORY build)

# Set compilation options
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} -std=c++11 -Wall -pthread -lm -g -O3")
set(CMAKE_C_FLAGS "-w -pthread -g -O3")

target_link_libraries(client isal m)
# target_link_libraries(test isal m)
target_link_libraries(storagenodes isal m)
target_link_libraries(pswitch isal m)