SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${OpenGL_INCLUDE_DIRS})

ADD_LIBRARY(rasterizer
	ras_mesh.h
	ras_rasterizer.h
	ras_vertex.h

	ras_mesh.cpp
	ras_rasterizer.cpp
	)

