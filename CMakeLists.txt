cmake_minimum_required(VERSION 3.16)
project(povray LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

option(ENABLE_ANTLR_GENERATION "Generate ANTLR C++ sources into build/ at build time" OFF)
option(ENABLE_ANTLR_CPP_RUNTIME "Compile ANTLR-generated C++ sources into a bridge target" OFF)
set(ANTLR4_JAR "" CACHE FILEPATH "Path to antlr-4.x-complete.jar")
option(ENABLE_ANTLR_DOWNLOAD "Auto-download ANTLR jar when not provided" OFF)
set(ANTLR4_VERSION "4.9.3" CACHE STRING "ANTLR version used for code generation")
set(ANTLR_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/antlr")
set(ANTLR_GRAMMAR_DIR "${CMAKE_SOURCE_DIR}/grammar")
set(ANTLR4_RUNTIME_INCLUDE_DIR "" CACHE PATH "Path to antlr4-runtime C++ headers")
set(ANTLR4_RUNTIME_LIBRARY "" CACHE FILEPATH "Path to antlr4-runtime C++ library")

if(ENABLE_ANTLR_GENERATION)
  if(NOT EXISTS "${ANTLR4_JAR}" AND ENABLE_ANTLR_DOWNLOAD)
    set(ANTLR_TOOLS_DIR "${CMAKE_BINARY_DIR}/tools/antlr")
    file(MAKE_DIRECTORY "${ANTLR_TOOLS_DIR}")
    set(ANTLR4_JAR "${ANTLR_TOOLS_DIR}/antlr-${ANTLR4_VERSION}-complete.jar")
    if(NOT EXISTS "${ANTLR4_JAR}")
      message(STATUS "Downloading ANTLR ${ANTLR4_VERSION} to ${ANTLR4_JAR}")
      file(DOWNLOAD
        "https://www.antlr.org/download/antlr-${ANTLR4_VERSION}-complete.jar"
        "${ANTLR4_JAR}"
        SHOW_PROGRESS
        STATUS ANTLR_DOWNLOAD_STATUS
        TLS_VERIFY ON
      )
      list(GET ANTLR_DOWNLOAD_STATUS 0 ANTLR_DOWNLOAD_CODE)
      if(NOT ANTLR_DOWNLOAD_CODE EQUAL 0)
        list(GET ANTLR_DOWNLOAD_STATUS 1 ANTLR_DOWNLOAD_MESSAGE)
        message(FATAL_ERROR "Failed to download ANTLR jar: ${ANTLR_DOWNLOAD_MESSAGE}")
      endif()
    endif()
  endif()

  if(NOT EXISTS "${ANTLR4_JAR}")
    message(FATAL_ERROR
      "ENABLE_ANTLR_GENERATION=ON requires ANTLR4_JAR, or set ENABLE_ANTLR_DOWNLOAD=ON")
  endif()

  find_package(Java REQUIRED COMPONENTS Runtime)
  file(MAKE_DIRECTORY "${ANTLR_OUTPUT_DIR}")

  set(ANTLR_LEXER_GRAMMAR "${ANTLR_GRAMMAR_DIR}/POVLexer.g4")
  set(ANTLR_PARSER_GRAMMAR "${ANTLR_GRAMMAR_DIR}/POVParser.g4")
  set(ANTLR_LEXER_STAMP "${ANTLR_OUTPUT_DIR}/POVLexer.stamp")
  set(ANTLR_PARSER_STAMP "${ANTLR_OUTPUT_DIR}/POVParser.stamp")

  add_custom_command(
    OUTPUT "${ANTLR_LEXER_STAMP}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR4_JAR}"
      -Dlanguage=Cpp
      -visitor
      -no-listener
      -Xexact-output-dir
      -o "${ANTLR_OUTPUT_DIR}"
      "${ANTLR_LEXER_GRAMMAR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${ANTLR_LEXER_STAMP}"
    BYPRODUCTS
      "${ANTLR_OUTPUT_DIR}/POVLexer.cpp"
      "${ANTLR_OUTPUT_DIR}/POVLexer.h"
      "${ANTLR_OUTPUT_DIR}/POVLexer.interp"
      "${ANTLR_OUTPUT_DIR}/POVLexer.tokens"
    DEPENDS "${ANTLR_LEXER_GRAMMAR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating ANTLR lexer from POVLexer.g4"
    VERBATIM
  )

  add_custom_command(
    OUTPUT "${ANTLR_PARSER_STAMP}"
    COMMAND "${Java_JAVA_EXECUTABLE}" -jar "${ANTLR4_JAR}"
      -Dlanguage=Cpp
      -visitor
      -no-listener
      -Xexact-output-dir
      -lib "${ANTLR_OUTPUT_DIR}"
      -o "${ANTLR_OUTPUT_DIR}"
      "${ANTLR_PARSER_GRAMMAR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${ANTLR_PARSER_STAMP}"
    BYPRODUCTS
      "${ANTLR_OUTPUT_DIR}/POVParser.cpp"
      "${ANTLR_OUTPUT_DIR}/POVParser.h"
      "${ANTLR_OUTPUT_DIR}/POVParser.interp"
      "${ANTLR_OUTPUT_DIR}/POVParser.tokens"
      "${ANTLR_OUTPUT_DIR}/POVParserBaseVisitor.h"
      "${ANTLR_OUTPUT_DIR}/POVParserVisitor.h"
    DEPENDS
      "${ANTLR_PARSER_GRAMMAR}"
      "${ANTLR_LEXER_GRAMMAR}"
      "${ANTLR_OUTPUT_DIR}/POVLexer.tokens"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating ANTLR parser from POVParser.g4"
    VERBATIM
  )

  add_custom_target(antlr_generate
    DEPENDS
      "${ANTLR_LEXER_STAMP}"
      "${ANTLR_PARSER_STAMP}"
  )

  if(ENABLE_ANTLR_CPP_RUNTIME)
    if(NOT EXISTS "${ANTLR4_RUNTIME_INCLUDE_DIR}")
      message(FATAL_ERROR
        "ENABLE_ANTLR_CPP_RUNTIME=ON requires ANTLR4_RUNTIME_INCLUDE_DIR")
    endif()
    if(NOT EXISTS "${ANTLR4_RUNTIME_LIBRARY}")
      message(FATAL_ERROR
        "ENABLE_ANTLR_CPP_RUNTIME=ON requires ANTLR4_RUNTIME_LIBRARY")
    endif()

    add_library(pov_antlr_bridge STATIC
      "${ANTLR_OUTPUT_DIR}/POVLexer.cpp"
      "${ANTLR_OUTPUT_DIR}/POVParser.cpp"
      src/io/pov/antlr/AntlrParseTreeToIrMapper.cpp
      src/io/pov/antlr/AntlrSceneParserFrontend.cpp
      src/io/pov/antlr/AntlrSceneIr.cpp
      src/io/pov/antlr/AntlrParsedSceneProgram.cpp
      src/io/pov/antlr/AntlrSceneIrBuilder.cpp
      src/io/pov/antlr/AntlrSceneLowering.cpp
      src/environment/camera/Camera.cpp
      src/environment/material/RenderRuntimeState.cpp
      src/common/linealAlgebra/Transformation.cpp
      src/common/logger/Logger.cpp
      src/environment/scene/ModelBuilder.cpp
      src/environment/scene/SimpleBodyFactory.cpp
      src/media/Texture.cpp
    )
    add_dependencies(pov_antlr_bridge antlr_generate)
    target_include_directories(pov_antlr_bridge
      PUBLIC
        src
        "${ANTLR_OUTPUT_DIR}"
        "${ANTLR4_RUNTIME_INCLUDE_DIR}"
    )
    target_compile_definitions(pov_antlr_bridge PRIVATE POV_WITH_ANTLR_RUNTIME=1)
    target_compile_definitions(pov_antlr_bridge PRIVATE POV_ANTLR_MINIMAL_BRIDGE=1)
    target_link_libraries(pov_antlr_bridge PUBLIC "${ANTLR4_RUNTIME_LIBRARY}")
    target_compile_features(pov_antlr_bridge PUBLIC cxx_std_11)

    add_executable(pov_antlr_smoke
      src/io/pov/antlr/AntlrSmokeTest.cpp
    )
    add_dependencies(pov_antlr_smoke pov_antlr_bridge)
    target_include_directories(pov_antlr_smoke PRIVATE src "${ANTLR_OUTPUT_DIR}" "${ANTLR4_RUNTIME_INCLUDE_DIR}")
    target_link_libraries(pov_antlr_smoke PRIVATE pov_antlr_bridge "${ANTLR4_RUNTIME_LIBRARY}")
    target_compile_definitions(pov_antlr_smoke PRIVATE POV_WITH_ANTLR_RUNTIME=1)
    target_compile_features(pov_antlr_smoke PRIVATE cxx_std_11)
  endif()
endif()

add_executable(povray
  src/app/main.cpp
  src/app/PovrayApplication.cpp
  src/common/color/Color.cpp
  src/common/logger/Logger.cpp
  src/common/linealAlgebra/Transformation.cpp
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
  src/io/image/DumpFormat.cpp
  src/io/image/GifFormat.cpp
  src/io/image/GifDecoder.cpp
  src/io/image/IffFormat.cpp
  src/io/pov/Parse.cpp
  src/io/pov/ParserContext.cpp
  src/io/pov/SymbolTable.cpp
  src/io/pov/TokenizerTokenStream.cpp
  src/io/pov/ParseErrorReporter.cpp
  src/io/pov/ParseHelpers.cpp
  src/io/pov/PrimitiveParser.cpp
  src/io/pov/mediaParser/TextureParser.cpp
  src/io/pov/mediaParser/ColorMapParser.cpp
  src/io/pov/geometryParser/SphereParser.cpp
  src/io/pov/lightParser/LightSourceParser.cpp
  src/io/pov/geometryParser/PlaneParser.cpp
  src/io/pov/geometryParser/TriangleParser.cpp
  src/io/pov/geometryParser/SmoothTriangleParser.cpp
  src/io/pov/geometryParser/QuadricParser.cpp
  src/io/pov/geometryParser/PolyParser.cpp
  src/io/pov/geometryParser/BicubicPatchParser.cpp
  src/io/pov/geometryParser/BoxParser.cpp
  src/io/pov/geometryParser/BlobParser.cpp
  src/io/pov/geometryParser/HeightFieldParser.cpp
  src/io/pov/geometryParser/ObjectParser.cpp
  src/io/pov/SceneConfigParser.cpp
  src/io/pov/SceneParser.cpp
  src/io/pov/FogParser.cpp
  src/io/pov/mediaParser/DefaultTextureParser.cpp
  src/io/pov/RenderSettingsParser.cpp
  src/io/pov/cameraParser/CameraParser.cpp
  src/io/pov/DeclarationParser.cpp
  src/io/pov/SceneFrameParser.cpp
  src/io/pov/ast/AstPrimitiveParser.cpp
  src/io/pov/ast/AstNodes.cpp
  src/io/pov/ast/AstObjectParser.cpp
  src/io/pov/ast/AstParsedSceneProgram.cpp
  src/io/pov/ast/AstSceneParser.cpp
  src/io/pov/ast/AstSceneBuilder.cpp
  src/io/pov/antlr/AntlrSceneIr.cpp
  src/io/pov/antlr/AntlrParsedSceneProgram.cpp
  src/io/pov/antlr/AntlrSceneIrBuilder.cpp
  src/io/pov/antlr/AntlrSceneParserFrontend.cpp
  src/io/pov/antlr/AntlrSceneLowering.cpp
  src/io/pov/antlr/AntlrParseDiagnostics.cpp
  src/io/pov/antlr/AntlrSceneRuntimePipeline.cpp
  src/io/image/RawFormat.cpp
  src/io/image/TargaFormat.cpp
  src/io/FileLocator.cpp
  src/io/Tokenizer.cpp
  src/media/Texture.cpp
  src/media/BumpTextureFixture.cpp
  src/media/ColorTextureFixture.cpp
  src/media/MapTextureFixture.cpp
  src/media/TextureFixture.cpp
  src/processing/PolynomialSolver.cpp
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
  src/environment/material/RendererConfiguration.cpp
  src/environment/material/RenderRuntimeState.cpp
  src/common/Statistics.cpp
  src/java/lang/Object.cpp
  src/java/lang/String.cpp
  src/java/util/Formatter.cpp
  src/java/io/InputStream.cpp
  src/java/io/OutputStream.cpp
  src/java/io/File.cpp
  src/java/io/FileInputStream.cpp
  src/java/io/FileOutputStream.cpp
  src/io/PersistenceElement.cpp
)

target_include_directories(povray PRIVATE src)
target_compile_options(povray PRIVATE -O3 -Wall -pedantic)

target_link_libraries(povray PRIVATE m)

if(ENABLE_ANTLR_GENERATION)
  add_dependencies(povray antlr_generate)
  target_include_directories(povray PRIVATE "${ANTLR_OUTPUT_DIR}")
endif()
