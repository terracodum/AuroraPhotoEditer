find_package(Qt5 COMPONENTS Core Gui Test Quick Concurrent REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test_pipeline
    test_pipeline.cpp
    ../src/PipelineManager.cpp
    ../src/BackgroundWorker.cpp
    ../src/MLProfiler.cpp
    ../src/MLInferenceEngine.cpp
    ../src/MLTensorProcessor.cpp
    ../src/EnhanceCommand.cpp
    ../src/BackgroundCommand.cpp
)

target_include_directories(test_pipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${AURORA_INCLUDE_DIRS}
)

target_link_libraries(test_pipeline PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Test
    Qt5::Concurrent
    Qt5::Quick
    ${AURORA_LDFLAGS}
    PkgConfig::ONNXRUNTIME
    PkgConfig::OPENCV
)

add_test(NAME test_pipeline COMMAND test_pipeline)
