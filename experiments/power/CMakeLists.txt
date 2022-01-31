file(GLOB powerSources *.cpp *.c)
file(GLOB powerIncludes *.h )

# compile the program
cuda_add_executable(power ${recognitionSources})
# link the jetson-inference lib
# target_link_libraries(recognition jetson-inference)
target_link_libraries(power profiling)
install(TARGETS power DESTINATION bin)
