cmake_minimum_required(VERSION 3.24)
project(VisionFlowZXingProbe LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/VisionDependencies.cmake")
visionflow_find_zxing_reader("${VISIONFLOW_ZXING_PREFIX}")
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
add_executable(zxing_reader_probe zxing_reader_probe.cpp)
target_include_directories(zxing_reader_probe PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(zxing_reader_probe PRIVATE ZXing::ZXing ${OpenCV_LIBS})
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/zxing-link-inventory.txt" CONTENT
"compiler=${CMAKE_CXX_COMPILER}\ncompilerVersion=${CMAKE_CXX_COMPILER_VERSION}\nbuildType=${CMAKE_BUILD_TYPE}\nstandard=${CMAKE_CXX_STANDARD}\npointerSize=${CMAKE_SIZEOF_VOID_P}\nreader=$<TARGET_FILE:ZXing::ZXing>\nOpenCV_VERSION=${OpenCV_VERSION}\n")
enable_testing()
add_test(NAME zxing_reader_probe COMMAND zxing_reader_probe
  "${CMAKE_CURRENT_LIST_DIR}/../dependency-fixtures/codes.tsv"
  "${VISIONFLOW_CODE_FIXTURES}" "${CMAKE_BINARY_DIR}/zxing-results.json")
set_tests_properties(zxing_reader_probe PROPERTIES TIMEOUT 60)
