cmake_minimum_required(VERSION 3.16)
project(vitral_raycasting LANGUAGES CXX)

if(NOT TARGET vitral_base)
    set(VITRAL_CPP_ROOT "$ENV{HOME}/VITRAL/vitral/cpp" CACHE PATH "VITRAL cpp checkout")
    add_subdirectory("${VITRAL_CPP_ROOT}/base" "${CMAKE_CURRENT_BINARY_DIR}/base")
endif()

add_library(vitral_raycasting STATIC
    src/main/vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/element/RayWithTracingState.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/element/SphereTrace.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/surface/RayCastingInfinitePlane.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/surface/parametric/ParametricBiCubicIntersection.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/surface/parametric/ParametricBiCubicSolver.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/surface/parametric/ParametricPatch.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/Blob.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/HeightField.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/BoxInvertedCrossingSink.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/RayCastingBox.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByMorganRules.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometryByRaySegment.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/CsgOperand.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/RaySegments.cpp
    src/main/vsdk/toolkit/environment/raycasting/geometry/volume/polynomial/PolynomialShape.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/RayCastingBody.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/RayCastingCoreOperand.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/NativeGeometryCrossingSink.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/ProjectedTriangleTraversal.cpp
    src/main/vsdk/toolkit/environment/raycasting/scene/RayCastingComposite.cpp
)
target_compile_features(vitral_raycasting PUBLIC cxx_std_11)
target_include_directories(vitral_raycasting PUBLIC src/main)
target_link_libraries(vitral_raycasting PUBLIC vitral_base)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(vitral_raycasting PRIVATE -O3 -Wall -pedantic)
endif()

option(RAYCASTING_BUILD_TESTS "Build raycasting integration tests" ON)
if(RAYCASTING_BUILD_TESTS)
    enable_testing()
    add_executable(vitral_raycasting_tests tests/sharedEnvironment.cpp
        tests/TestSlabGeometry.cpp tests/TestCrossingCollector.cpp)
    target_link_libraries(vitral_raycasting_tests PRIVATE vitral_raycasting)
    add_test(NAME raycasting_shared_environment COMMAND vitral_raycasting_tests)
endif()
