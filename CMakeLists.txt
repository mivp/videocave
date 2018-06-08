cmake_minimum_required(VERSION 2.8.3) 

project(videocave)

message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

include(FindPkgConfig)

# OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

# OpenMPI
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

# Set output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set shader path
add_definitions(-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/shaders/\")

include_directories(
	external/glfw-3.0.3/include/GLFW/
	external/glew-1.9.0/include/
	external/glm-0.9.4.0/
	include/
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	GLFW_303
	GLEW_190
	${MPI_LIBRARIES}
	-lm 
	-ldl 
	-lpthread
)

# Compile apps
add_subdirectory (external)

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)


SET( sources 
	GLUtils.cpp
	Texture.cpp
	GLSLProgram.cpp
	Material.cpp
	SSQuad.cpp
	Display.cpp
	Capture.cpp 
	Config.cpp 
	include/DeckLinkAPIDispatch.cpp
	#main.cpp
)

SET( headers 
	GLInclude.h
	GLUtils.h
	Texture.h
	GLSLProgram.h
	Material.h
	SSQuad.h
	Display.h
	stb_image.h
	Capture.h
	Config.h 
)

# Build common
add_executable( videocave ${sources} ${headers} )
target_link_libraries( videocave ${ALL_LIBS} )