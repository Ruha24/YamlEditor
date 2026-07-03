cmake_minimum_required(VERSION 3.16)

project(editor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Concurrent)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)


include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0
  GIT_SHALLOW TRUE
)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)


set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/mainwindow.ui
        src/api/yandex/yandexapi.h
        src/api/yandex/yandexapi.cpp
        src/files/yaml/yamlreader.h
        src/files/yaml/yamlreader.cpp
        src/files/yaml/yamlnode.h
        src/files/yaml/yamlnode.cpp
        src/searchingwindow.h
        src/searchingwindow.cpp
        src/searchingwindow.ui
        src/files/filesystem.h
        src/files/filesystem.cpp
        src/replacewindow.h
        src/replacewindow.cpp
        src/replacewindow.ui
        src/customWidget/customlineedit.h
        src/customWidget/customlineedit.cpp
        src/editwidget.h
        src/editwidget.cpp
        src/editwidget.ui
        src/files/filedownloadtask.h
        src/files/filedownloadtask.cpp
        src/customWidget/flowlayout.h
        src/customWidget/flowlayout.cpp
        src/customWidget/logpanel.h
        src/customWidget/logpanel.cpp
        src/images.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(editor
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

qt_add_translations(editor TS_FILES
    src/translations/editor_en_GB.ts
    src/translations/editor_ru_RU.ts
)
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET editor APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(editor SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(editor
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(editor PRIVATE Qt${QT_VERSION_MAJOR}::Widgets yaml-cpp::yaml-cpp Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Concurrent)

set_target_properties(editor PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS editor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(editor)
endif()

# --- Unit tests -------------------------------------------------------------
# Build with:  cmake -DBUILD_TESTING=ON ..  &&  ctest --output-on-failure
option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
