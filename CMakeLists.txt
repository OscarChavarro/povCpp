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
  src/common/Matrices.cpp
  src/common/Ray.cpp
  src/geom/Bezier.cpp
  src/geom/BezierIntersection.cpp
  src/geom/BezierPatches.cpp
  src/geom/Blob.cpp
  src/geom/Boxes.cpp
  src/geom/Csg.cpp
  src/geom/HField.cpp
  src/geom/Light.cpp
  src/geom/Objects.cpp
  src/geom/Planes.cpp
  src/geom/Poly.cpp
  src/geom/PrioQ.cpp
  src/geom/Quadrics.cpp
  src/geom/Spheres.cpp
  src/geom/Triangle.cpp
  src/geom/ViewPnt.cpp
  src/io/Dump.cpp
  src/io/Gif.cpp
  src/io/GifDecod.cpp
  src/io/Iff.cpp
  src/io/pov/Parse.cpp
  src/io/pov/ParseErrorReporter.cpp
  src/io/pov/ParseHelpers.cpp
  src/io/pov/PrimitiveParser.cpp
  src/io/pov/TextureParser.cpp
  src/io/pov/ColorMapParser.cpp
  src/io/pov/SphereParser.cpp
  src/io/pov/LightSourceParser.cpp
  src/io/pov/PlaneParser.cpp
  src/io/pov/TriangleParser.cpp
  src/io/pov/SmoothTriangleParser.cpp
  src/io/pov/QuadricParser.cpp
  src/io/pov/PolyParser.cpp
  src/io/pov/BicubicPatchParser.cpp
  src/io/pov/BoxParser.cpp
  src/io/pov/BlobParser.cpp
  src/io/pov/HeightFieldParser.cpp
  src/io/pov/ObjectParser.cpp
  src/io/pov/SceneConfigParser.cpp
  src/io/pov/SceneParser.cpp
  src/io/pov/SceneFactory.cpp
  src/io/pov/FogParser.cpp
  src/io/pov/DefaultTextureParser.cpp
  src/io/pov/RenderSettingsParser.cpp
  src/io/pov/ViewPointParser.cpp
  src/io/pov/DeclarationParser.cpp
  src/io/pov/SceneFrameParser.cpp
  src/io/Raw.cpp
  src/io/Targa.cpp
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
