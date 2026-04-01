#    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
#    Copyright (C) 2010-2015 — IanniX Association
#
#    IanniX is a free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    any later version.

cmake_minimum_required(VERSION 3.17)

project(IanniX
    VERSION   1.0.0
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# Optional features (disabled by default, matching legacy .pro `false` blocks)
# ---------------------------------------------------------------------------
option(USE_FFMPEG  "Enable FFmpeg recording support (requires libav*)" OFF)
option(USE_KINECT  "Enable Kinect support (requires libfreenect)"      OFF)
option(USE_WACOM   "Enable Wacom tablet support (macOS only)"          OFF)

# ---------------------------------------------------------------------------
# Qt version selection
# ---------------------------------------------------------------------------
set(QT_VERSION "5" CACHE STRING "Qt major version to build against (5 or 6)")
set_property(CACHE QT_VERSION PROPERTY STRINGS 5 6)

if(QT_VERSION STREQUAL "6")
    # --- Qt6 stub ---
    # Qt6 removed the QtScript module.  Full Qt6 support requires migrating
    # QScriptEngine usage to QJSEngine (from Qt6::Qml).  Until that work is
    # done this path will fail at link time for any code that touches
    # QScriptEngine.  It is provided so that the rest of the build
    # infrastructure can be validated against Qt6.
    message(WARNING
        "Qt6 support is experimental. QScriptEngine has been removed in Qt6; "
        "the build will fail until the QScript->QJSEngine migration is complete."
    )
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Xml OpenGL SerialPort WebSockets)
    find_package(Qt6 QUIET COMPONENTS Qml)
    add_compile_definitions(QT6)
    set(QT_LIBS
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Xml Qt6::OpenGL Qt6::SerialPort Qt6::WebSockets
    )
    if(Qt6Qml_FOUND)
        list(APPEND QT_LIBS Qt6::Qml)
    endif()
else()
    # --- Qt5 (default, fully supported) ---
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Script Xml OpenGL SerialPort WebSockets)
    add_compile_definitions(QT5)
    set(QT_LIBS
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Script Qt5::Xml Qt5::OpenGL Qt5::SerialPort Qt5::WebSockets
    )
endif()

# ---------------------------------------------------------------------------
# Platform compile definitions
# ---------------------------------------------------------------------------
if(APPLE)
    add_compile_definitions(__MACOSX_CORE__ IANNIX_64 SYPHON_INSTALLED)
elseif(WIN32)
    add_compile_definitions(__WINDOWS_MM__ IANNIX_64 IANNIX_32)
elseif(UNIX)
    add_compile_definitions(IANNIX_64 IANNIX_32)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_compile_definitions(__LINUX_ALSA__ __linux__ AVOID_TIMESTAMPING)
    endif()
endif()

# RtMidi debug diagnostics in Debug builds
add_compile_definitions($<$<CONFIG:Debug>:__RTMIDI_DEBUG__>)

# ---------------------------------------------------------------------------
# Source file groups
# ---------------------------------------------------------------------------

# -- Core & Misc --
set(CORE_HEADERS
    iannix.h iannixapp.h iannix_spec.h iannix_cmd.h
    misc/help.h misc/application.h misc/options.h misc/applicationexecute.h
)
set(CORE_SOURCES
    iannix.cpp iannixapp.cpp iannix_spec.cpp
    misc/help.cpp misc/application.cpp misc/options.cpp misc/applicationexecute.cpp
)

# -- Messages --
set(MESSAGES_HEADERS
    messages/messagemanagerlogmini.h messages/messagemanagerlog.h
    messages/messagemanager.h messages/message.h messages/messagemanagerloginterface.h
)
set(MESSAGES_SOURCES
    messages/messagemanagerlogmini.cpp messages/messagemanagerlog.cpp
    messages/messagemanager.cpp messages/message.cpp
)
set(MESSAGES_FORMS
    messages/messagemanagerlogmini.ui messages/messagemanagerlog.ui
)

# -- Transport --
set(TRANSPORT_HEADERS
    transport/transport.h transport/uitimer.h transport/uiabout.h transport/uieditor.h
)
set(TRANSPORT_SOURCES
    transport/transport.cpp transport/uitimer.cpp transport/uiabout.cpp transport/uieditor.cpp
)
set(TRANSPORT_FORMS
    transport/transport.ui transport/uitimer.ui transport/uiabout.ui transport/uieditor.ui
)

# -- Render --
set(RENDER_HEADERS
    render/uirender.h render/uirenderpreview.h
)
set(RENDER_SOURCES
    render/uirender.cpp render/uirenderpreview.cpp
)
set(RENDER_FORMS
    render/uirender.ui
)

# -- Geometry (includes vendored muParser) --
set(GEOMETRY_HEADERS
    geometry/nxpoint.h geometry/nxrect.h geometry/nxsize.h
    geometry/nxline.h geometry/nxpolygon.h geometry/nxeasing.h
    geometry/qmuparser/muParser.h geometry/qmuparser/muParserBase.h
    geometry/qmuparser/muParserBytecode.h geometry/qmuparser/muParserCallback.h
    geometry/qmuparser/muParserError.h geometry/qmuparser/muParserTokenReader.h
    geometry/qmuparser/muParserDef.h geometry/qmuparser/muParserFixes.h
    geometry/qmuparser/muParserStack.h geometry/qmuparser/muParserToken.h
)
set(GEOMETRY_SOURCES
    geometry/nxpoint.cpp geometry/nxrect.cpp geometry/nxsize.cpp
    geometry/nxline.cpp geometry/nxpolygon.cpp geometry/nxeasing.cpp
    geometry/qmuparser/muParser.cpp geometry/qmuparser/muParserBase.cpp
    geometry/qmuparser/muParserBytecode.cpp geometry/qmuparser/muParserCallback.cpp
    geometry/qmuparser/muParserError.cpp geometry/qmuparser/muParserTokenReader.cpp
)

# -- Objects --
set(OBJECTS_HEADERS
    objects/nxdocument.h objects/nxtrigger.h objects/nxgroup.h
    objects/nxcurve.h objects/nxcursor.h objects/nxobject.h
)
set(OBJECTS_SOURCES
    objects/nxdocument.cpp objects/nxtrigger.cpp objects/nxgroup.cpp
    objects/nxcurve.cpp objects/nxcursor.cpp objects/nxobject.cpp
)

# -- GUI (includes vendored jsedit) --
set(GUI_HEADERS
    gui/uiinspector.h gui/uiview.h gui/uihelp.h gui/uimessagebox.h gui/uisplashscreen.h
    gui/qjsedit/jsedit.h
)
set(GUI_SOURCES
    gui/uiinspector.cpp gui/uiview.cpp gui/uihelp.cpp gui/uimessagebox.cpp gui/uisplashscreen.cpp
    gui/qjsedit/jsedit.cpp
)
set(GUI_FORMS
    gui/uiinspector.ui gui/uiview.ui gui/uihelp.ui gui/uimessagebox.ui
)

# -- Items --
set(ITEMS_HEADERS
    items/uitreeview.h items/uitreeviewwidget.h items/uitreedelegate.h
    items/uifileitem.h items/uicoloritem.h items/uipathpointsitem.h items/uitextureitem.h
)
set(ITEMS_SOURCES
    items/uitreeview.cpp items/uitreeviewwidget.cpp items/uitreedelegate.cpp
    items/uifileitem.cpp items/uicoloritem.cpp items/uipathpointsitem.cpp items/uitextureitem.cpp
)
set(ITEMS_FORMS
    items/uitreeview.ui
)

# -- Interfaces (ext dialogs + native protocol handlers) --
set(INTERFACES_HEADERS
    interfaces/extscriptvariableask.h interfaces/extoscpatternask.h interfaces/extoscpatterneditor.h
    interfaces/interfacehttp.h interfaces/interfacemidi.h interfaces/interfaceosc.h
    interfaces/interfaceserial.h interfaces/interfacetcp.h interfaces/interfaceudp.h
    interfaces/interfacedirect.h interfaces/interfacesyphon.h
)
set(INTERFACES_SOURCES
    interfaces/extscriptvariableask.cpp interfaces/extoscpatternask.cpp interfaces/extoscpatterneditor.cpp
    interfaces/interfacehttp.cpp interfaces/interfacemidi.cpp interfaces/interfaceosc.cpp
    interfaces/interfaceserial.cpp interfaces/interfacetcp.cpp interfaces/interfaceudp.cpp
    interfaces/interfacedirect.cpp
)
set(INTERFACES_FORMS
    interfaces/extscriptvariableask.ui interfaces/extoscpatternask.ui interfaces/extoscpatterneditor.ui
    interfaces/interfacehttp.ui interfaces/interfacemidi.ui interfaces/interfaceosc.ui
    interfaces/interfaceserial.ui interfaces/interfacetcp.ui interfaces/interfaceudp.ui
    interfaces/interfacedirect.ui interfaces/interfacesyphon.ui
)

# -- Vendored: RtMidi --
set(RTMIDI_HEADERS
    interfaces/qrtmidi/RtMidi.h interfaces/qrtmidi/RtError.h
)
set(RTMIDI_SOURCES
    interfaces/qrtmidi/RtMidi.cpp
)

# -- AbstractionsGL --
set(ABSTRACTIONS_HEADERS abstractionsgl.h)
set(ABSTRACTIONS_SOURCES abstractionsgl.cpp)

# ---------------------------------------------------------------------------
# Platform-conditional sources
# ---------------------------------------------------------------------------

# macOS-only: Artnet
set(ARTNET_HEADERS "")
set(ARTNET_SOURCES "")
if(APPLE)
    set(ARTNET_HEADERS
        interfaces/artnet/artnet.h interfaces/artnet/misc.h interfaces/artnet/packets.h
        interfaces/artnet/private.h interfaces/artnet/tod.h interfaces/artnet/common.h
    )
    set(ARTNET_SOURCES
        interfaces/artnet/artnet.c interfaces/artnet/misc.c interfaces/artnet/network.c
        interfaces/artnet/receive.c interfaces/artnet/tod.c interfaces/artnet/transmit.c
    )
endif()

# macOS-only: Zeroconf / Bonjour
set(ZEROCONF_HEADERS "")
set(ZEROCONF_SOURCES "")
if(APPLE)
    set(ZEROCONF_HEADERS
        interfaces/zeroconf/bonjourserviceregister.h
        interfaces/zeroconf/bonjourserviceresolver.h
        interfaces/zeroconf/bonjourservicebrowser.h
        interfaces/zeroconf/bonjourrecord.h
    )
    set(ZEROCONF_SOURCES
        interfaces/zeroconf/bonjourserviceregister.cpp
        interfaces/zeroconf/bonjourserviceresolver.cpp
        interfaces/zeroconf/bonjourservicebrowser.cpp
    )
endif()

# macOS-only: Syphon (Objective-C++)
set(SYPHON_SOURCES "")
if(APPLE)
    set(SYPHON_SOURCES interfaces/interfacesyphon.mm)
    set_source_files_properties(interfaces/interfacesyphon.mm
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
endif()

# ---------------------------------------------------------------------------
# Optional features
# ---------------------------------------------------------------------------

set(OPTIONAL_HEADERS "")
set(OPTIONAL_SOURCES "")
set(OPTIONAL_INCLUDES "")
set(OPTIONAL_LIBS "")

# -- FFmpeg --
if(USE_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec libavutil libavformat libswscale
    )
    add_compile_definitions(FFMPEG_INSTALLED)
    if(WIN32)
        add_compile_definitions(__STDC_CONSTANT_MACROS)
    endif()
    list(APPEND OPTIONAL_HEADERS
        gui/qffmpeg/QVideoEncoder.h gui/qffmpeg/QVideoDecoder.h
    )
    list(APPEND OPTIONAL_SOURCES
        gui/qffmpeg/QVideoEncoder.cpp gui/qffmpeg/QVideoDecoder.cpp
    )
    list(APPEND OPTIONAL_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/gui/qffmpeg)
    list(APPEND OPTIONAL_LIBS PkgConfig::FFMPEG)
endif()

# -- Kinect --
if(USE_KINECT AND UNIX)
    add_compile_definitions(KINECT_INSTALLED)
    list(APPEND OPTIONAL_HEADERS
        interfaces/qkinect/QKinect.h interfaces/extkinectmanager.h
    )
    list(APPEND OPTIONAL_SOURCES
        interfaces/qkinect/QKinect.cpp interfaces/extkinectmanager.cpp
    )
    list(APPEND OPTIONAL_INCLUDES /usr/local/include/libfreenect)
    list(APPEND OPTIONAL_LIBS freenect)
endif()

# -- Wacom (macOS only) --
if(USE_WACOM AND APPLE)
    add_compile_definitions(WACOM_INSTALLED)
    list(APPEND OPTIONAL_HEADERS interfaces/extwacommanager.h)
    list(APPEND OPTIONAL_SOURCES
        interfaces/extwacommanager.mm
        interfaces/qwacom/WacomTabletDriver.m
    )
    set_source_files_properties(
        interfaces/extwacommanager.mm
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
    set_source_files_properties(
        interfaces/qwacom/WacomTabletDriver.m
        PROPERTIES COMPILE_FLAGS "-x objective-c"
    )
endif()

# ---------------------------------------------------------------------------
# Aggregate all sources
# ---------------------------------------------------------------------------
set(ALL_SOURCES
    ${CORE_SOURCES}        ${CORE_HEADERS}
    ${MESSAGES_SOURCES}    ${MESSAGES_HEADERS}
    ${TRANSPORT_SOURCES}   ${TRANSPORT_HEADERS}
    ${RENDER_SOURCES}      ${RENDER_HEADERS}
    ${GEOMETRY_SOURCES}    ${GEOMETRY_HEADERS}
    ${OBJECTS_SOURCES}     ${OBJECTS_HEADERS}
    ${GUI_SOURCES}         ${GUI_HEADERS}
    ${ITEMS_SOURCES}       ${ITEMS_HEADERS}
    ${INTERFACES_SOURCES}  ${INTERFACES_HEADERS}
    ${RTMIDI_SOURCES}      ${RTMIDI_HEADERS}
    ${ABSTRACTIONS_SOURCES} ${ABSTRACTIONS_HEADERS}
    ${ARTNET_SOURCES}      ${ARTNET_HEADERS}
    ${ZEROCONF_SOURCES}    ${ZEROCONF_HEADERS}
    ${SYPHON_SOURCES}
    ${OPTIONAL_SOURCES}    ${OPTIONAL_HEADERS}
)

set(ALL_FORMS
    ${MESSAGES_FORMS}
    ${TRANSPORT_FORMS}
    ${RENDER_FORMS}
    ${GUI_FORMS}
    ${ITEMS_FORMS}
    ${INTERFACES_FORMS}
)

set(RESOURCES
    icons/IanniX.qrc
)

# ---------------------------------------------------------------------------
# Executable target
# ---------------------------------------------------------------------------
if(APPLE)
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        ${ALL_SOURCES} ${ALL_FORMS} ${RESOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${ALL_SOURCES} ${ALL_FORMS} ${RESOURCES}
    )
endif()

# On Linux the binary is lowercase per convention
if(UNIX AND NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "iannix")
endif()

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/misc
    ${CMAKE_CURRENT_SOURCE_DIR}/messages
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/render
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/qmuparser
    ${CMAKE_CURRENT_SOURCE_DIR}/objects
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/qjsedit
    ${CMAKE_CURRENT_SOURCE_DIR}/items
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/qrtmidi
    ${OPTIONAL_INCLUDES}
)

if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/artnet
        ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/zeroconf
    )
endif()

# ---------------------------------------------------------------------------
# Link Qt
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_LIBS} ${OPTIONAL_LIBS})

# ---------------------------------------------------------------------------
# Platform-specific libraries
# ---------------------------------------------------------------------------
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreMidi"
        "-framework CoreAudio"
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework Carbon"
        "-framework Cocoa"
        "-framework Syphon"
    )
    target_link_options(${PROJECT_NAME} PRIVATE -F/Library/Frameworks)

elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm setupapi advapi32 user32 opengl32
    )
    target_sources(${PROJECT_NAME} PRIVATE icon.rc)

elseif(UNIX)
    # OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # ALSA (required for RtMidi on Linux)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ALSA REQUIRED alsa)
        target_include_directories(${PROJECT_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ALSA_LIBRARIES})

    endif()
endif()

# ---------------------------------------------------------------------------
# macOS bundle configuration
# ---------------------------------------------------------------------------
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        MACOSX_BUNDLE_ICON_FILE  "icon.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "IanniX"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.iannix.desktop"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )

    set(BUNDLE_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/icon.icns")
    set(BUNDLE_ICON_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/icon_file.icns")
    set_source_files_properties(${BUNDLE_ICON} ${BUNDLE_ICON_FILE}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )
    target_sources(${PROJECT_NAME} PRIVATE ${BUNDLE_ICON} ${BUNDLE_ICON_FILE})

    # Bundle resource directories (Examples, Tools, Patches)
    foreach(_dir Examples Tools Patches)
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}")
            file(GLOB_RECURSE _files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${_dir}/*")
            foreach(_file ${_files})
                get_filename_component(_file_dir "${_file}" DIRECTORY)
                set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${_file}"
                    PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_file_dir}"
                )
                target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
            endforeach()
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Translations (optional, requires LinguistTools)
# ---------------------------------------------------------------------------
if(QT_VERSION STREQUAL "6")
    find_package(Qt6LinguistTools QUIET)
    if(Qt6LinguistTools_FOUND)
        set(TS_FILES Tools/Translation_fr_FR.ts)
        qt6_add_translation(QM_FILES ${TS_FILES})
        target_sources(${PROJECT_NAME} PRIVATE ${QM_FILES})
    endif()
else()
    find_package(Qt5LinguistTools QUIET)
    if(Qt5LinguistTools_FOUND)
        set(TS_FILES Tools/Translation_fr_FR.ts)
        qt5_add_translation(QM_FILES ${TS_FILES})
        target_sources(${PROJECT_NAME} PRIVATE ${QM_FILES})
    endif()
endif()

# ---------------------------------------------------------------------------
# Install rules (Linux / generic Unix)
# ---------------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES iannix.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
    install(FILES iannix.png     DESTINATION /usr/share/pixmaps)
endif()

# ---------------------------------------------------------------------------
# Build summary
# ---------------------------------------------------------------------------
if(QT_VERSION STREQUAL "6")
    set(_qt_ver "${Qt6_VERSION}")
else()
    set(_qt_ver "${Qt5_VERSION}")
endif()
message(STATUS "-----------------------------------------------")
message(STATUS "IanniX ${PROJECT_VERSION}")
message(STATUS "  System     : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Generator  : ${CMAKE_GENERATOR}")
message(STATUS "  Qt         : ${_qt_ver} (Qt${QT_VERSION})")
message(STATUS "  C++ std    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  FFmpeg     : ${USE_FFMPEG}")
message(STATUS "  Kinect     : ${USE_KINECT}")
message(STATUS "  Wacom      : ${USE_WACOM}")
if(UNIX AND NOT APPLE)
    message(STATUS "  ALSA       : ${ALSA_FOUND}")
endif()
message(STATUS "-----------------------------------------------")
