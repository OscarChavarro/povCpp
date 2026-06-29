cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

add_subdirectory(base)

add_executable(povray
    src/common/statistics/Statistics.cpp
    src/environment/material/povray/PovRayMaterial.cpp
    src/io/pov/material/PovRayMaterialUtils.cpp
    src/io/pov/material/PovRayMaterialBuilder.cpp
    src/environment/material/povray/PovRayMaterialConstancy.cpp
    src/environment/material/normal/BumpMapNormal.cpp
    src/environment/material/normal/BumpsNormal.cpp
    src/environment/material/normal/DentsNormal.cpp
    src/environment/material/normal/RipplesNormal.cpp
    src/environment/material/normal/UnsupportedBumpNormal.cpp
    src/environment/material/normal/WavesNormal.cpp
    src/environment/material/normal/WrinklesNormal.cpp
    src/environment/material/pigment/AgatePigment.cpp
    src/environment/material/pigment/BozoPigment.cpp
    src/environment/material/pigment/BrickPigment.cpp
    src/environment/material/pigment/CheckerColorPigment.cpp
    src/environment/material/pigment/CheckerTexturePigment.cpp
    src/environment/material/pigment/ColourPigment.cpp
    src/environment/material/pigment/GradientPigment.cpp
    src/environment/material/pigment/GranitePigment.cpp
    src/environment/material/pigment/ImageMapPigment.cpp
    src/environment/material/pigment/LeopardPigment.cpp
    src/environment/material/pigment/MarblePigment.cpp
    src/environment/material/pigment/MaterialMapPigment.cpp
    src/environment/material/pigment/OnionPigment.cpp
    src/environment/material/pigment/SolidTexturePigment.cpp
    src/environment/material/pigment/SpottedPigment.cpp
    src/environment/material/pigment/WoodPigment.cpp
    src/environment/material/PovRayRendererConfiguration.cpp
    src/io/pov/material/DefaultTextureAliasTracker.cpp
    src/environment/geometry/element/RayWithSegments.cpp
    src/environment/geometry/surface/parametric/ParametricPatch.cpp
    src/environment/geometry/surface/parametric/ParametricBiCubicIntersection.cpp
    src/environment/geometry/surface/parametric/ParametricBiCubicSolver.cpp
    src/environment/geometry/volume/Blob.cpp
    src/environment/geometry/volume/Box.cpp
    src/environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.cpp
    src/environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.cpp
    src/environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.cpp
    src/environment/geometry/volume/constructiveSolidGeometry/CsgOperand.cpp
    src/environment/geometry/volume/constructiveSolidGeometry/RaySegments.cpp
    src/environment/geometry/volume/HeightField.cpp
    src/environment/geometry/Geometry.cpp
    src/environment/geometry/surface/InfinitePlane.cpp
    src/environment/geometry/volume/polynomial/PolynomialShape.cpp
    src/environment/geometry/volume/Quadric.cpp
    src/environment/geometry/volume/Sphere.cpp
    src/environment/geometry/element/Triangle.cpp
    src/environment/light/Light.cpp
    src/environment/light/PointLight.cpp
    src/environment/light/SpotLight.cpp
    src/environment/scene/SimpleBody.cpp
    src/environment/scene/Composite.cpp
    src/environment/scene/Scene.cpp
    src/io/pov/geometry/SimpleBodyBuilder.cpp
    src/io/pov/geometry/SceneBuilder.cpp
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
    src/render/AdaptiveAntiAliasing.cpp
    src/render/ColorOperations.cpp
    src/render/RenderWorker.cpp
    src/render/RenderTargetImage.cpp
    src/render/RayShaderPipeline.cpp
    src/render/RenderEngine.cpp
    src/render/RenderImageWriter.cpp
    src/io/context/RenderRuntimeState.cpp
    src/io/binaryIo/FileLocator.cpp
    src/io/image/RawDumpFormat.cpp
    src/io/image/GifFormat.cpp
    src/io/image/GifDecoder.cpp
    src/io/image/IffFormat.cpp
    src/io/image/RawFormat.cpp
    src/io/image/TargaFormat.cpp
    src/io/pov/light/LightGeometryAdapter.cpp
    src/io/pov/geometry/GeometryBuilder.cpp
    src/io/pov/context/ParserContext.cpp
    src/io/pov/context/SymbolTable.cpp
    src/io/pov/context/TokenizerStream.cpp
    src/io/pov/parser/ParseErrorReporter.cpp
    src/io/pov/parser/ParseHelpers.cpp
    src/io/pov/parser/PrimitiveParser.cpp
    src/io/pov/material/PovColorMap.cpp
    src/io/pov/material/PovColorMapSpan.cpp
    src/io/pov/material/TextureParser.cpp
    src/io/pov/material/ColorMapParser.cpp
    src/io/pov/material/DefaultTextureParser.cpp
    src/io/pov/material/LoadedImageRegistry.cpp
    src/io/pov/geometry/SphereParser.cpp
    src/io/pov/light/LightSourceParser.cpp
    src/io/pov/geometry/PlaneParser.cpp
    src/io/pov/geometry/TriangleParser.cpp
    src/io/pov/geometry/QuadricParser.cpp
    src/io/pov/geometry/PolyParser.cpp
    src/io/pov/geometry/BicubicPatchParser.cpp
    src/io/pov/geometry/BoxParser.cpp
    src/io/pov/geometry/BlobParser.cpp
    src/io/pov/geometry/CsgParser.cpp
    src/io/pov/geometry/HeightFieldParser.cpp
    src/io/pov/geometry/ObjectParser.cpp
    src/io/pov/scene/SceneParser.cpp
    src/io/pov/scene/ScenePostProcessor.cpp
    src/io/pov/scene/FogParser.cpp
    src/io/pov/scene/RenderSettingsParser.cpp
    src/io/pov/camera/PovCameraSpec.cpp
    src/io/pov/camera/CameraParser.cpp
    src/io/pov/scene/DeclarationParser.cpp
    src/io/pov/scene/SceneBodyParser.cpp
    src/io/pov/lexer/Tokenizer.cpp
    src/app/ImageOutputAdapter.cpp
    src/app/options/CommandLineOptions.cpp
    src/app/PovRayApplication.cpp
    src/app/main.cpp
)

target_include_directories(povray PRIVATE
    ${CMAKE_SOURCE_DIR}/base/src/main
    ${CMAKE_SOURCE_DIR}/base/src
    src
)
target_compile_options(povray PRIVATE -O3 -Wall -pedantic)

target_link_libraries(povray PRIVATE vitral_base m)
