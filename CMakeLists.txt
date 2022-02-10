cmake_minimum_required(VERSION 3.11.0)
project(TraceApp VERSION 1.0)

# Compile flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_GENERATOR_PLATFORM x64)

include("src/TraceClient/CMakeLists.txt")