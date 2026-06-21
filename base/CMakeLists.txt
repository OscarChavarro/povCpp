cmake_minimum_required(VERSION 3.16)
project(vitral_base_cpp LANGUAGES CXX)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/build")

set(VITRAL_BASE_SOURCES
    src/main/vsdk/toolkit/common/VSDK.cpp
    src/main/vsdk/toolkit/common/logging/Logger.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Matrix4x4d.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/AlgebraPrimitives.cpp
    src/main/java/lang/Object.cpp
    src/main/java/lang/String.cpp
    src/main/java/lang/System.cpp
    src/main/java/io/InputStream.cpp
    src/main/java/io/File.cpp
    src/main/java/io/FileInputStream.cpp
    src/main/java/io/FileOutputStream.cpp
    src/main/java/io/PrintStream.cpp
    src/main/java/util/Formatter.cpp
    src/main/vsdk/toolkit/io/PersistenceElement.cpp
    src/main/vsdk/toolkit/media/RGBAImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/media/RGBAColorPalette.cpp
    src/main/vsdk/toolkit/media/IndexedColorImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/environment/geometry/element/Ray.cpp
    src/main/vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableSine.cpp
    src/main/vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableChecksum16.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/QuadraticSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/CubicSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/QuarticSolver.cpp
    src/main/vsdk/toolkit/numericalAnalysis/polynomial/PolynomialSolver.cpp
    src/main/vsdk/toolkit/media/solidTexture/TextureUtils.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/BumpTextureFixture.cpp
    src/main/vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/SolidTextureCoordinateMapper.cpp
    src/main/vsdk/toolkit/media/solidTexture/from2d/ImageTexture.cpp
    src/main/vsdk/toolkit/render/raytracing/RasterTileArea.cpp
    src/main/vsdk/toolkit/render/raytracing/RasterTileGenerator.cpp
    src/main/java/util/concurrent/Executors.cpp
    src/main/vsdk/toolkit/common/statistics/SolidTextureStatistics.cpp
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ArrayListInstantiations.cpp" [=[
#include "java/util/ArrayList.h"
#include "java/util/ArrayList.txx"
#include "java/lang/String.h"

namespace java {
template class ArrayList<String>;
template class ArrayList<unsigned char>;
template class ArrayList<double>;
}
]=])

list(APPEND VITRAL_BASE_SOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/ArrayListInstantiations.cpp"
)

add_library(vitral_base SHARED ${VITRAL_BASE_SOURCES})

target_include_directories(vitral_base PUBLIC src src/main)

# Real pthreads are always available on the Linux build this project targets,
# and the parallel raytracer (-parallel) needs java::Executors backed by real
# threads, so this is enabled unconditionally rather than gated behind a
# CMake option that compile.sh would have to know to pass.
find_package(Threads REQUIRED)
target_compile_definitions(vitral_base PUBLIC VITRAL_WITH_POSIX_THREADS)
target_link_libraries(vitral_base PUBLIC Threads::Threads)
