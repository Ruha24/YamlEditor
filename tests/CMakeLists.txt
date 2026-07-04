find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test Widgets)

qt_add_library(yamleditor_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/files/yaml/yamlnode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/files/yaml/yamlreader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/files/filesystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/files/yaml/yamlvalidator.cpp
)
target_include_directories(yamleditor_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(yamleditor_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    yaml-cpp::yaml-cpp
)

function(add_core_test test_name test_source)
    qt_add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE
        yamleditor_core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_core_test(tst_yamlnode    tst_yamlnode.cpp)
add_core_test(tst_yamlreader  tst_yamlreader.cpp)
add_core_test(tst_filesystem  tst_filesystem.cpp)
add_core_test(tst_yamlvalidator tst_yamlvalidator.cpp)

set_tests_properties(tst_filesystem PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
