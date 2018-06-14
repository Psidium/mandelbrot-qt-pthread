cmake_minimum_required(VERSION 3.0) # setting this is required
project(example_project)            # this sets the project name

###############################################################################
## file globbing ##############################################################
###############################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables 
# `sources` and `data`.
file(GLOB_RECURSE sources      src/main/*.cpp src/main/*.h)
file(GLOB_RECURSE sources_test src/test/*.cpp)
file(GLOB_RECURSE data resources/*)
file(GLOB_RECURSE mandelbrot_func vendor/mandelbrot/mandelbrot.*)
# You can use set(sources src/main.cpp) etc if you don't want to
# use globbing to find files automatically.

###############################################################################
## target definitions #########################################################
###############################################################################

# The data is just added to the executable, because in some IDEs (QtCreator) 
# files are invisible when they are not explicitly part of the project.
add_executable(parallel_mandelbrot ${sources} ${mandelbrot_func} ${data} )

# Just for parallel_mandelbrot add some compiler flags.
set(CMAKE_CXX_FLAGS -std=c++1y -Wall -Wfloat-conversion)
target_compile_options(parallel_mandelbrot PUBLIC )

# This allows to include files relative to the root of the src directory with a <> pair
target_include_directories(parallel_mandelbrot PUBLIC src/main vendor/mandelbrot)

# This copies all resource files in the build directory.
# We need this, because we want to work with paths relative to the executable.
file(COPY ${data} DESTINATION resources)

###############################################################################
## dependencies ###############################################################
###############################################################################

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# This defines the variables Boost_LIBRARIES that containts all library names
# that we need to link into the program.
find_package(Boost 1.36.0 COMPONENTS filesystem system REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

QT5_WRAP_CPP(parallel_mandelbrot_SRC src/main/QtObjects.h )
target_sources(parallel_mandelbrot PUBLIC ${parallel_mandelbrot_SRC})

include_directories(${Qt5_INCLUDES})
add_definitions(${Qt5_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${Qt5_EXECUTABLE_COMPILE_FLAGS}")

target_link_libraries(parallel_mandelbrot PUBLIC
  ${Boost_LIBRARIES}
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui
  pthread
)

###############################################################################
## testing ####################################################################
###############################################################################


# This is for our testing framework,
# we don't add REQUIRED because it's just for testing.
# People who might want to build the project to use it should not be required
# to install testing dependencies.

add_subdirectory( vendor/googletest/googletest )

add_executable(unit_tests ${sources_test} ${sources} ${mandelbrot_func})

# This define is added to prevent collision with the main.
# It might be better solved by not adding the source with the main to the
# testing target.
target_compile_definitions(unit_tests PUBLIC UNIT_TESTS)

# This allows us to use the executable as a link library, and inherit all 
# linker options and library dependencies from it, by simply adding it as dependency.
set_target_properties(parallel_mandelbrot PROPERTIES ENABLE_EXPORTS on)

target_link_libraries(unit_tests PUBLIC
gtest_main
parallel_mandelbrot
)

###############################################################################
## packaging ##################################################################
###############################################################################

# All install commands get the same destination. this allows us to use paths
# relative to the executable.
install(TARGETS parallel_mandelbrot DESTINATION example_destination)

# This is basically a repeat of the file copy instruction that copies the
# resources in the build directory, but here we tell cmake that we want it
# in the package.
install(DIRECTORY resources DESTINATION example_destination)

# Now comes everything we need, to create a package
# there are a lot more variables you can set, and some
# you need to set for some package types, but we want to
# be minimal here.
set(CPACK_PACKAGE_NAME "MyExample")
set(CPACK_PACKAGE_VERSION "1.0.0")

# We don't want to split our program up into several incomplete pieces.
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
