# copy source files
file(GLOB powerProfilingSources *.cpp)
file(GLOB powerProfilingIncludes *h)

# compile the program
cuda_add_executable(power_profiler ${powerProfilingSources})

# link our profiling lib (contains jetson-inference and jetson-utils)
target_link_libraries(power_profiler profiling)
# install executable in bin folder
install(TARGETS power_profiler DESTINATION bin)
