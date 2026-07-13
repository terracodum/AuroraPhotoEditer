find_package(Qt5 COMPONENTS Core Gui Test REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test_pipeline
    test_pipeline.cpp
    ../src/PipelineManager.cpp
)

target_include_directories(test_pipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_pipeline PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Test
)

add_test(NAME test_pipeline COMMAND test_pipeline)
