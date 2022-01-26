file(GLOB recognitionSources *.cpp)
file(GLOB recognitionIncludes *.h )

# compile the program
cuda_add_executable(recognition ${recognitionSources})
# link the jetson-inference lib
# target_link_libraries(recognition jetson-inference)
target_link_libraries(recognition profiling)
install(TARGETS recognition DESTINATION bin)
