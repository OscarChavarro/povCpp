cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_subdirectory(base)

add_executable(povray
  src/app/main.cpp
  src/app/ImageOutputAdapter.cpp
  src/app/options/CommandLineOptions.cpp
  src/app/PovrayApplication.cpp
  src/render/ColorOperations.cpp
  src/io/pov/texture/PovColorMap.cpp
  src/environment/geometry/elements/Ray.cpp
  src/environment/geometry/elements/RayWithSegments.cpp
  src/environment/geometry/surface/parametric/ParametricPatch.cpp
  src/environment/geometry/surface/parametric/ParametricBiCubicIntersection.cpp
  src/environment/geometry/surface/parametric/ParametricBiCubicSolver.cpp
  src/environment/geometry/volume/Blob.cpp
  src/environment/geometry/volume/Box.cpp
  src/environment/geometry/volume/compound/CSG.cpp
  src/environment/geometry/volume/HeightField.cpp
  src/environment/light/Light.cpp
  src/environment/geometry/volume/compound/Composite.cpp
  src/environment/geometry/surface/InfinitePlane.cpp
  src/environment/geometry/volume/polynomial/PolynomialShape.cpp
  src/environment/geometry/volume/Quadric.cpp
  src/environment/geometry/volume/Sphere.cpp
  src/environment/scene/ModelBuilder.cpp
  src/environment/scene/SimpleBodyFactory.cpp
  src/environment/geometry/elements/Triangle.cpp
  src/environment/camera/Camera.cpp
  src/io/image/RawDumpFormat.cpp
  src/io/image/GifFormat.cpp
  src/io/image/GifDecoder.cpp
  src/io/image/IffFormat.cpp
  src/io/pov/context/ParserContext.cpp
  src/io/pov/context/SymbolTable.cpp
  src/io/pov/context/TokenizerStream.cpp
  src/io/pov/parser/ParseErrorReporter.cpp
  src/io/pov/parser/ParseHelpers.cpp
  src/io/pov/parser/PrimitiveParser.cpp
  src/io/pov/texture/TextureParser.cpp
  src/io/pov/texture/ColorMapParser.cpp
  src/io/pov/geometry/SphereParser.cpp
  src/io/pov/light/LightSourceParser.cpp
  src/io/pov/geometry/PlaneParser.cpp
  src/io/pov/geometry/TriangleParser.cpp
  src/io/pov/geometry/SmoothTriangleParser.cpp
  src/io/pov/geometry/QuadricParser.cpp
  src/io/pov/geometry/PolyParser.cpp
  src/io/pov/geometry/BicubicPatchParser.cpp
  src/io/pov/geometry/BoxParser.cpp
  src/io/pov/geometry/BlobParser.cpp
  src/io/pov/geometry/HeightFieldParser.cpp
  src/io/pov/geometry/ObjectParser.cpp
  src/io/pov/scene/SceneParser.cpp
  src/io/pov/scene/FogParser.cpp
  src/io/pov/texture/DefaultTextureParser.cpp
  src/io/pov/scene/RenderSettingsParser.cpp
  src/io/pov/camera/CameraParser.cpp
  src/io/pov/scene/DeclarationParser.cpp
  src/io/pov/scene/SceneFrameParser.cpp
  src/io/image/RawFormat.cpp
  src/io/image/TargaFormat.cpp
  src/io/binaryIo/FileLocator.cpp
  src/io/pov/lexer/Tokenizer.cpp
  src/solidTexture/TextureUtils.cpp
  src/solidTexture/ProceduralNoise.cpp
  src/solidTexture/BumpTextureFixture.cpp
  src/solidTexture/ColorTextureFixture.cpp
  src/solidTexture/FixturesFacade.cpp
  src/solidTexture/MapTextureFixture.cpp
  src/solidTexture/TextureFixture.cpp
  src/processing/polynomial/PolynomialSolver.cpp
  src/processing/polynomial/PolynomialTermCounts.cpp
  src/render/shaders/LightSamplerShader.cpp
  src/render/shaders/ShadowShader.cpp
  src/render/shaders/PhongSpecularShader.cpp
  src/render/shaders/BlinnPhongSpecularShader.cpp
  src/render/shaders/LambertShader.cpp
  src/render/shaders/ExponentialFogShader.cpp
  src/render/shaders/BumpNormalShader.cpp
  src/render/shaders/AmbientLightShader.cpp
  src/render/shaders/DirectLightShader.cpp
  src/render/shaders/MirrorReflectionShader.cpp
  src/render/shaders/TransmissionRefractionShader.cpp
  src/render/shaders/LocalSurfaceShader.cpp
  src/render/RayShaderPipeline.cpp
  src/render/RenderEngine.cpp
  src/render/SceneDump.cpp
  src/environment/material/RendererConfiguration.cpp
  src/common/RenderRuntimeState.cpp
  src/common/statistics/Statistics.cpp
)

target_include_directories(povray PRIVATE
  ${CMAKE_SOURCE_DIR}/base/src/main
  ${CMAKE_SOURCE_DIR}/base/src
  src
)
target_compile_options(povray PRIVATE -O3 -Wall -pedantic)

target_link_libraries(povray PRIVATE vitral_base m)
