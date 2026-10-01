cmake_minimum_required(VERSION 3.16)
project(remarkable-sp VERSION 0.3.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Quick Network)
# Private QPA API to inject pen events (src/peninput.cpp); since Qt 6.10 a
# separate package, before part of Qt6Gui.
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

set(CORE_SOURCES
    src/stroke.h src/stroke.cpp
    src/inkcanvas.h src/inkcanvas.cpp
    src/spstore.h src/spstore.cpp
    src/workspace.h src/workspace.cpp
    src/syncfile.h src/syncfile.cpp
    src/syncbackend.h src/syncbackend.cpp
    src/syncengine.h src/syncengine.cpp
    src/updater.h src/updater.cpp
    src/icon.h src/icon.cpp
    src/peninput.h src/peninput.cpp
)

add_library(rmsp_core STATIC ${CORE_SOURCES})
target_include_directories(rmsp_core PUBLIC src)
target_link_libraries(rmsp_core PUBLIC Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::Network)
target_compile_definitions(rmsp_core PUBLIC RMSP_VERSION="${PROJECT_VERSION}")

find_package(ZLIB REQUIRED)
target_link_libraries(rmsp_core PUBLIC ZLIB::ZLIB)

# Encrypted sync (Argon2id + AES-256-GCM, as in Super Productivity).
# Argon2 is vendored (reference implementation), AES comes from OpenSSL.
option(RMSP_ENABLE_CRYPTO "Support encrypted sync data" ON)
if(RMSP_ENABLE_CRYPTO)
    find_package(OpenSSL COMPONENTS Crypto)
    if(OpenSSL_FOUND)
        add_library(argon2 STATIC
            third_party/argon2/src/argon2.c third_party/argon2/src/core.c
            third_party/argon2/src/encoding.c third_party/argon2/src/ref.c
            third_party/argon2/src/thread.c third_party/argon2/src/blake2/blake2b.c)
        target_include_directories(argon2 PUBLIC third_party/argon2/include PRIVATE third_party/argon2/src)
        target_compile_definitions(argon2 PRIVATE ARGON2_NO_THREADS)
        set_target_properties(argon2 PROPERTIES POSITION_INDEPENDENT_CODE ON C_STANDARD 99)
        target_link_libraries(rmsp_core PUBLIC argon2 OpenSSL::Crypto)
        target_compile_definitions(rmsp_core PUBLIC RMSP_HAVE_CRYPTO)
    else()
        message(WARNING "OpenSSL not found: encrypted sync disabled")
    endif()
endif()

qt_add_executable(remarkable-sp src/main.cpp)
target_link_libraries(remarkable-sp PRIVATE rmsp_core)
qt_add_resources(remarkable-sp "fonts"
    PREFIX "/fonts"
    BASE "third_party/fonts"
    FILES third_party/fonts/NotoSans-Regular.ttf third_party/fonts/NotoSans-Bold.ttf)

# Release asset the in-app updater looks for (see src/updater.cpp).
set(RMSP_UPDATE_ASSET "" CACHE STRING "Updater asset name, e.g. remarkable-sp-rm1")
if(RMSP_UPDATE_ASSET)
    target_compile_definitions(rmsp_core PUBLIC RMSP_UPDATE_ASSET="${RMSP_UPDATE_ASSET}")
endif()

set(RMSP_QML_FILES
    qml/Main.qml
    qml/Theme.qml
    qml/TaskList.qml
    qml/TaskRow.qml
    qml/TaskDetail.qml
    qml/InkField.qml
    qml/IconButton.qml
    qml/RmButton.qml
    qml/Chip.qml
    qml/Popover.qml
    qml/Field.qml
    qml/Keyboard.qml
    qml/NavRail.qml
    qml/TopBar.qml
    qml/SettingsPage.qml
    qml/WorklogPage.qml
)
# Flatten qml/ into the module root so every file sees the module's qmldir
# (needed for the Theme singleton) via the implicit directory import.
foreach(f ${RMSP_QML_FILES})
    get_filename_component(name ${f} NAME)
    set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${name})
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(remarkable-sp
    URI RemarkableSP
    VERSION 1.0
    QML_FILES ${RMSP_QML_FILES}
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
