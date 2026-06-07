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
    src/main/vsdk/toolkit/common/color/ColorRgb.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Complex.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/MatrixNxM.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Matrix4x4d.cpp
    src/main/vsdk/toolkit/common/linealAlgebra/Matrix4x4f.cpp
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
    src/main/vsdk/toolkit/linealAlgebra/processing/LinearAlgebraEngine.cpp
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ArrayListInstantiations.cpp" [=[
#include "java/util/ArrayList.h"
#include "java/util/ArrayList.txx"
#include "java/lang/String.h"

namespace java {
template class ArrayList<String>;
template class ArrayList<unsigned char>;
}
]=])

list(APPEND VITRAL_BASE_SOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/ArrayListInstantiations.cpp"
)

add_library(vitral_base SHARED ${VITRAL_BASE_SOURCES})

target_include_directories(vitral_base PUBLIC src src/main)

if(WITH_POSIX_THREADS)
    target_compile_definitions(vitral_base PUBLIC VITRAL_WITH_POSIX_THREADS)
    target_link_libraries(vitral_base PUBLIC pthread)
endif()
