cmake_minimum_required(VERSION 3.16)
project(remarkable-sp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Quick Network)

set(CORE_SOURCES
    src/stroke.h src/stroke.cpp
    src/inkcanvas.h src/inkcanvas.cpp
    src/task.h src/task.cpp
    src/taskmodel.h src/taskmodel.cpp
    src/spimport.h src/spimport.cpp
    src/updater.h src/updater.cpp
)

add_library(rmsp_core STATIC ${CORE_SOURCES})
target_include_directories(rmsp_core PUBLIC src)
target_link_libraries(rmsp_core PUBLIC Qt6::Core Qt6::Gui Qt6::Quick Qt6::Network)
target_compile_definitions(rmsp_core PUBLIC RMSP_VERSION="${PROJECT_VERSION}")

qt_add_executable(remarkable-sp src/main.cpp)
target_link_libraries(remarkable-sp PRIVATE rmsp_core)

qt_add_qml_module(remarkable-sp
    URI RemarkableSP
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/TaskList.qml
        qml/TaskRow.qml
        qml/TaskDetail.qml
        qml/InkField.qml
        qml/EButton.qml
        qml/Header.qml
)

install(TARGETS remarkable-sp RUNTIME DESTINATION bin)

option(RMSP_BUILD_TESTS "Build unit tests" ON)
if(RMSP_BUILD_TESTS)
    find_package(Qt6 COMPONENTS Test)
    if(Qt6Test_FOUND)
        enable_testing()
        add_executable(rmsp_tests tests/tst_core.cpp)
        target_link_libraries(rmsp_tests PRIVATE rmsp_core Qt6::Test)
        add_test(NAME rmsp_tests COMMAND rmsp_tests)
    endif()
endif()
