find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

set(YAML_MODEL_SOURCES
    ${CMAKE_SOURCE_DIR}/src/files/yaml/yamlnode.cpp
    ${CMAKE_SOURCE_DIR}/src/files/yaml/yamlreader.cpp
)

function(add_core_test test_name test_source)
    qt_add_executable(${test_name} ${test_source} ${YAML_MODEL_SOURCES})
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${test_name} PRIVATE
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Core
        yaml-cpp::yaml-cpp
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_core_test(tst_yamlnode    tst_yamlnode.cpp)
add_core_test(tst_yamlreader  tst_yamlreader.cpp)

qt_add_executable(tst_filesystem
    tst_filesystem.cpp
    ${CMAKE_SOURCE_DIR}/src/files/filesystem.cpp
)
target_include_directories(tst_filesystem PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tst_filesystem PRIVATE
    Qt${QT_VERSION_MAJOR}::Test
    Qt${QT_VERSION_MAJOR}::Widgets
    yaml-cpp::yaml-cpp
)
add_test(NAME tst_filesystem COMMAND tst_filesystem)
