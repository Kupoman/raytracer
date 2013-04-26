
#########################################################
# FIND ASSIMP
#########################################################
find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS})
add_definitions(${ASSIMP_DEFINITIONS})
if(NOT ASSIMP_FOUND)
    message(ERROR " ASSIMP not found!")
endif(NOT ASSIMP_FOUND)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(data STATIC
			camera.h
			data.h
			loader.h
			scene.h
			
			camera.cpp
			data.cpp
			loader.cpp
			scene.cpp)

target_link_libraries(data ${ASSIMP_LIBRARIES})
