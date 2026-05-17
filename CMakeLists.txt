cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_executable(povray
  src/app/Povray.cpp
  src/app/UnixPlatform.cpp
  src/common/Color.cpp
  src/common/Transformation.cpp
  src/common/Ray.cpp
  src/geom/Bezier.cpp
  src/geom/BezierIntersection.cpp
  src/geom/BezierPatches.cpp
  src/geom/Blob.cpp
  src/geom/Box.cpp
  src/geom/CSG.cpp
  src/geom/HeightField.cpp
  src/geom/Light.cpp
  src/geom/Composite.cpp
  src/geom/InfinitePlane.cpp
  src/geom/PolynomialShape.cpp
  src/geom/PriorityQueue.cpp
  src/geom/PriorityQueuePool.cpp
  src/geom/Quadric.cpp
  src/geom/Sphere.cpp
  src/geom/Triangle.cpp
  src/geom/Viewpoint.cpp
  src/io/DumpFormat.cpp
  src/io/GifFormat.cpp
  src/io/GifDecoder.cpp
  src/io/IffFormat.cpp
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
  src/io/RawFormat.cpp
  src/io/TargaFormat.cpp
  src/io/Tokenizer.cpp
  src/media/Texture.cpp
  src/media/BumpTextures.cpp
  src/media/ColorTextures.cpp
  src/media/MapTextures.cpp
  src/media/TestTextures.cpp
  src/processing/PolynomialSolver.cpp
  src/render/LightingEngine.cpp
  src/render/RenderEngine.cpp
)

target_include_directories(povray PRIVATE src)
target_compile_options(povray PRIVATE -O3 -Wall -pedantic)

target_link_libraries(povray PRIVATE m)
