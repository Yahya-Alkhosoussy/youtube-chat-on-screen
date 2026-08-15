cmake_minimum_required(VERSION 3.19)
project(chat-display LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

set(Python3_ROOT_DIR "/Users/yahyaamr/Documents/GitHub/youtube-chat-on-screen/.venv")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

qt_add_executable(chat-display
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    preferences.cpp
    mainwindow.h
    preferences.h
    ChatResponse.h
    ChatWorker.h
    mainwindow.ui
    preferences.ui
    ChatWorker.cpp
)

target_link_libraries(chat-display
    PRIVATE
        Qt::Core
        Qt::Widgets
        pybind11::embed
)

install(TARGETS chat-display
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET chat-display
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
