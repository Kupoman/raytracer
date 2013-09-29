SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(raytracer STATIC
	rt_iaccel.h
	
	rt_accel_array.h
	rt_accel_spheres.h
	rt_photon_map.h
	rt_ray.h
	rt_raytracer.h
	
	rt_accel_array.cpp
	rt_accel_spheres.cpp
	rt_photon_map.cpp
	rt_raytracer.cpp
    rt_ray.cpp) 
