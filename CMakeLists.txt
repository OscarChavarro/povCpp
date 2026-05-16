cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_executable(povray
  src/app/Povray.cpp
  src/app/Unix.cpp
  src/common/Color.cpp
  src/common/RGBAColor.cpp
  src/common/RGBAColorPalette.cpp
  src/common/RGBAColorPaletteSpan.cpp
  src/common/Matrices.cpp
  src/common/Ray.cpp
  src/geom/Bezier.cpp
  src/geom/BezierChild.cpp
  src/geom/BezierNode.cpp
  src/geom/BezierVertices.cpp
  src/geom/BicubicPatch.cpp
  src/geom/Blob.cpp
  src/geom/BlobClass.cpp
  src/geom/BlobElement.cpp
  src/geom/BlobInterval.cpp
  src/geom/BlobList.cpp
  src/geom/Boxes.cpp
  src/geom/Csg.cpp
  src/geom/GeometryClass.cpp
  src/geom/HeightField.cpp
  src/geom/HeightFieldBlock.cpp
  src/geom/HField.cpp
  src/geom/Light.cpp
  src/geom/MethodsClass.cpp
  src/geom/Objects.cpp
  src/geom/Planes.cpp
  src/geom/Poly.cpp
  src/geom/PriorityQueue.cpp
  src/geom/PriorityQueueNodeClass.cpp
  src/geom/PrioQ.cpp
  src/geom/Quadrics.cpp
  src/geom/SimpleBody.cpp
  src/geom/SmoothTriangle.cpp
  src/geom/Spheres.cpp
  src/geom/TriangleClass.cpp
  src/geom/Triangle.cpp
  src/geom/ViewPnt.cpp
  src/io/Dump.cpp
  src/io/FileHandle.cpp
  src/io/Gif.cpp
  src/io/ChunkHeader.cpp
  src/io/GifDecod.cpp
  src/io/Iff.cpp
  src/io/Parse.cpp
  src/io/Raw.cpp
  src/io/Targa.cpp
  src/io/DataFile.cpp
  src/io/ReservedWord.cpp
  src/io/TokenStruct.cpp
  src/io/Tokenize.cpp
  src/media/Texture.cpp
  src/media/TxtBump.cpp
  src/media/TxtColor.cpp
  src/media/TxtMap.cpp
  src/media/TxtTest.cpp
  src/media/Vect.cpp
  src/render/Lighting.cpp
  src/render/Render.cpp
)

target_include_directories(povray PRIVATE src)
target_compile_options(povray PRIVATE -O3 -Wall -pedantic)

target_link_libraries(povray PRIVATE m)
