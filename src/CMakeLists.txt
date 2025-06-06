# CMakeList.txt : CMake project for Double Hashing Experiment, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (double-hashing "main.cpp" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET double-hashing PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
