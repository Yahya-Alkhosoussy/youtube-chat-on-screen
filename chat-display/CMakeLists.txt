cmake_minimum_required(VERSION 3.19)
project(chat-display LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

set(Python3_ROOT_DIR "${CMAKE_SOURCE_DIR}/../.venv")
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
if(APPLE)
    if(NOT EXISTS "/Library/Frameworks/Python.framework/Versions/3.12/Python")
        # only needed for the specific local install-name bug; skip if not applicable
    else()
        set(Python3_LIBRARY "/Library/Frameworks/Python.framework/Versions/3.12/Python")
    endif()
endif()

if(WIN32)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sys; print(sys.base_prefix)"
        OUTPUT_VARIABLE PYTHON_BASE_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    file(TO_CMAKE_PATH "${PYTHON_BASE_PREFIX}" PYTHON_BASE_PREFIX)
    file(GLOB PYTHON_RUNTIME_DLLS "${PYTHON_BASE_PREFIX}/python3*.dll")
    install(FILES ${PYTHON_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR})
    
    install(DIRECTORY "${PYTHON_BASE_PREFIX}/Lib/"
        DESTINATION python/Lib
        PATTERN "site-packages" EXCLUDE
        PATTERN "__pycache__" EXCLUDE
        PATTERN "test" EXCLUDE
    )

    install(DIRECTORY "${PYTHON_BASE_PREFIX}/DLLs/"
        DESTINATION python/DLLs
    )
endif()

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
    sounds.qrc
)

target_link_libraries(chat-display
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        pybind11::embed
)

if(APPLE)
    install(CODE "
        execute_process(COMMAND ditto
            \"$<TARGET_BUNDLE_DIR:chat-display>\"
            \"\${CMAKE_INSTALL_PREFIX}/chat-display.app\"
        )
    ")
else()
    install(TARGETS chat-display
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(APPLE)
    install(DIRECTORY "${Python3_ROOT_DIR}/lib/python3.12/site-packages/"
        DESTINATION chat-display.app/Contents/Resources/python/site-packages
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python/"
        DESTINATION chat-display.app/Contents/Resources/python/scripts
        PATTERN "token.json" EXCLUDE
        PATTERN "__pycache__" EXCLUDE
        PATTERN ".DS_Store" EXCLUDE
    )
elseif(WIN32)
    install(DIRECTORY "${Python3_ROOT_DIR}/Lib/site-packages/"
        DESTINATION python/site-packages
    )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/python/"
        DESTINATION python/scripts
    )
endif()

qt_generate_deploy_app_script(
    TARGET chat-display
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# Step 6 — CPack, must come last
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS")
endif()

if(WIN32)
    if(DEFINED VC_REDIST_INSTALLER AND EXISTS "${VC_REDIST_INSTALLER}")
        install(FILES "${VC_REDIST_INSTALLER}" DESTINATION redist)
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
            "ExecWait '\"$INSTDIR\\redist\\vc_redist.x64.exe\" /install /quiet /norestart'"
        )
    else()
        message(WARNING "VC_REDIST_INSTALLER not provided — Windows installer will not bundle the VC++ Redistributable")
    endif()
endif()

set(CPACK_PACKAGE_NAME "chat-display")
set(CPACK_PACKAGE_VERSION "1.0.0")
include(CPack)