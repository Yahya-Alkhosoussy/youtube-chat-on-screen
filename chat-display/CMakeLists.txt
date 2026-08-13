cmake_minimum_required(VERSION 3.19)
project(chat-display LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(chat-display
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    preferences.cpp
    mainwindow.h
    preferences.h
    mainwindow.ui
    preferences.ui
)

target_link_libraries(chat-display
    PRIVATE
        Qt::Core
        Qt::Widgets
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
