# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/low_level_test/fb_RGB666

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/low_level_test/fb_RGB666/build

# Include any dependencies generated for this target.
include CMakeFiles/fb_RGB666_bcm2835.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/fb_RGB666_bcm2835.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/fb_RGB666_bcm2835.dir/flags.make

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o: CMakeFiles/fb_RGB666_bcm2835.dir/flags.make
CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o: ../fb_RGB666_bcm2835.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/low_level_test/fb_RGB666/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o   -c /home/pi/low_level_test/fb_RGB666/fb_RGB666_bcm2835.c

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/low_level_test/fb_RGB666/fb_RGB666_bcm2835.c > CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.i

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/low_level_test/fb_RGB666/fb_RGB666_bcm2835.c -o CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.s

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.requires:

.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.requires

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.provides: CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.requires
	$(MAKE) -f CMakeFiles/fb_RGB666_bcm2835.dir/build.make CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.provides.build
.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.provides

CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.provides.build: CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o


# Object files for target fb_RGB666_bcm2835
fb_RGB666_bcm2835_OBJECTS = \
"CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o"

# External object files for target fb_RGB666_bcm2835
fb_RGB666_bcm2835_EXTERNAL_OBJECTS =

fb_RGB666_bcm2835: CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o
fb_RGB666_bcm2835: CMakeFiles/fb_RGB666_bcm2835.dir/build.make
fb_RGB666_bcm2835: CMakeFiles/fb_RGB666_bcm2835.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/low_level_test/fb_RGB666/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable fb_RGB666_bcm2835"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fb_RGB666_bcm2835.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/fb_RGB666_bcm2835.dir/build: fb_RGB666_bcm2835

.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/build

CMakeFiles/fb_RGB666_bcm2835.dir/requires: CMakeFiles/fb_RGB666_bcm2835.dir/fb_RGB666_bcm2835.c.o.requires

.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/requires

CMakeFiles/fb_RGB666_bcm2835.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/fb_RGB666_bcm2835.dir/cmake_clean.cmake
.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/clean

CMakeFiles/fb_RGB666_bcm2835.dir/depend:
	cd /home/pi/low_level_test/fb_RGB666/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/low_level_test/fb_RGB666 /home/pi/low_level_test/fb_RGB666 /home/pi/low_level_test/fb_RGB666/build /home/pi/low_level_test/fb_RGB666/build /home/pi/low_level_test/fb_RGB666/build/CMakeFiles/fb_RGB666_bcm2835.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/fb_RGB666_bcm2835.dir/depend
