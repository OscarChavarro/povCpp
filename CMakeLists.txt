cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_executable(povray
  src/app/povray.cpp
  src/app/unix.cpp
  src/common/colour.cpp
  src/common/matrices.cpp
  src/common/ray.cpp
  src/geom/bezier.cpp
  src/geom/blob.cpp
  src/geom/boxes.cpp
  src/geom/csg.cpp
  src/geom/hfield.cpp
  src/geom/light.cpp
  src/geom/objects.cpp
  src/geom/planes.cpp
  src/geom/poly.cpp
  src/geom/prioq.cpp
  src/geom/quadrics.cpp
  src/geom/spheres.cpp
  src/geom/triangle.cpp
  src/geom/viewpnt.cpp
  src/io/dump.cpp
  src/io/gif.cpp
  src/io/gifdecod.cpp
  src/io/iff.cpp
  src/io/parse.cpp
  src/io/raw.cpp
  src/io/targa.cpp
  src/io/tokenize.cpp
  src/media/texture.cpp
  src/media/txtbump.cpp
  src/media/txtcolor.cpp
  src/media/txtmap.cpp
  src/media/txttest.cpp
  src/media/vect.cpp
  src/render/lighting.cpp
  src/render/render.cpp
)

target_include_directories(povray PRIVATE src)
target_compile_options(povray PRIVATE -Wall -pedantic)

target_link_libraries(povray PRIVATE m)
