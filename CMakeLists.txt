cmake_minimum_required(VERSION 3.8)

# set project name
project(profiling)

# check that jetson shared libs are installed
find_package(jetson-utils)
find_package(jetson-inference)

# check thaht CUDA is installed
find_package(CUDA)
message("-- Found CUDA ${CUDA_VERSION}")

# setup build flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wno-write-strings -Wno-deprecated-declarations")

# setup project output paths
set(PROJECT_OUTPUT_DIR  ${PROJECT_BINARY_DIR}/${CMAKE_SYSTEM_PROCESSOR})
set(PROJECT_INCLUDE_DIR  ${PROJECT_OUTPUT_DIR}/include)

file(MAKE_DIRECTORY ${PROJECT_INCLUDE_DIR})
file(MAKE_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin)

message("-- system arch:  ${CMAKE_SYSTEM_PROCESSOR}")
message("-- output path:  ${PROJECT_OUTPUT_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR}/lib)

# build c/c++ libs
include_directories(${PROJECT_INCLUDE_DIR})

file(GLOB profilingSources src/*.cpp src/*.c)
file(GLOB profilingIncludes src/*.h)

# add directory for libnvbuf-utils to program
link_directories(/usr/lib/aarch64-linux-gnu/tegra)

add_library(profiling SHARED ${profilingSources})  # create the lib
target_link_libraries(profiling jetson-inference jetson-utils)

# transfer headers to the include directory
file(MAKE_DIRECTORY ${PROJECT_INCLUDE_DIR}/profiling)

foreach(include ${profilingIncludes})
	message("-- Copying ${include}")
	configure_file(${include} ${PROJECT_INCLUDE_DIR}/profiling COPYONLY)
endforeach()

# add subdirectories to build experiments
add_subdirectory(experiments)

# install includes
foreach(include  in ${profilingIncludes})
    install(FILES "${include}" DESTINATION include/profiling)
endforeach()

# install the shared lib
install(TARGETS profiling DESTINATION lib EXPORT profilingConfig)
# install the cmake project for importing
install(EXPORT profilingConfig DESTINATION share/profiling/cmake)

