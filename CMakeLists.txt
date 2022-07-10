cmake_minimum_required (VERSION 3.12)

project (ReflectHLSL VERSION 0.1.0 LANGUAGES CXX)

execute_process (COMMAND git submodule update --init --recursive)

add_subdirectory (parsegen)

add_executable (ReflectHLSL
	src/hlslp.cpp
	src/hlslp.hpp
	src/MetaData.cpp
	src/MetaData.hpp
	src/Generator.hpp
)

target_include_directories (ReflectHLSL PRIVATE parsegen/src)
target_include_directories (ReflectHLSL PRIVATE glm)

target_link_libraries (ReflectHLSL LINK_PUBLIC parsegen)

set_property (TARGET ReflectHLSL PROPERTY CXX_STANDARD 20)
