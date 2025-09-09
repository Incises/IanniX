cmake_minimum_required(VERSION 3.17)

# 仅写项目名称
project(IanniX)

# 单独指定项目版本变量
set(PROJECT_VERSION "1.0.0")

# 单独启用 C++ 语言
enable_language(CXX)

# ---------------------------
# 基本设置
# ---------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------
option(USE_FFMPEG "Enable FFmpeg support" OFF)
option(USE_TRACKPADS "Enable Wacom & Trackpads support" OFF)
option(USE_KINECT "Enable Kinect support" OFF)
# ---------------------------

# ---------------------------
# Qt 版本集中控制（仅此处）
# 只允许 5 或 6，默认 5
# ---------------------------
set(USE_QT_VERSION "5" CACHE STRING "Qt version to use: 5, 6")
set_property(CACHE USE_QT_VERSION PROPERTY STRINGS 5 6)

set(QT_LIBS "")

if(USE_QT_VERSION STREQUAL "5")
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Script Xml OpenGL)
    if(NOT Qt5_FOUND)
        message(FATAL_ERROR "Qt5 requested but not found")
    endif()
    add_compile_definitions(QT5)
    set(QT_LIBS Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Script Qt5::Xml Qt5::OpenGL)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
elseif(USE_QT_VERSION STREQUAL "6")
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Script Xml OpenGL)
    if(NOT Qt6_FOUND)
        message(FATAL_ERROR "Qt6 requested but not found")
    endif()
    add_compile_definitions(QT6)
    set(QT_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Script Qt6::Xml Qt6::OpenGL)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
else()
    message(FATAL_ERROR "Unsupported USE_QT_VERSION: ${USE_QT_VERSION} (allowed: 5,6)")
endif()

# ---------------------------
# 跨平台宏（一次性声明）
# ---------------------------
if(APPLE)
    add_compile_definitions(__MACOSX_CORE__)
elseif(WIN32)
    add_compile_definitions(__WINDOWS_MM__)
elseif(UNIX AND NOT APPLE)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        add_compile_definitions(__LINUX_ALSA__ __linux__)
    endif()
endif()

# ---------------------------
# 文件分组 —— 保留原路径与分组
# ---------------------------

# Core & Misc
set(CORE_HEADERS
    iannix.h iannixapp.h iannix_spec.h iannix_cmd.h
    misc/help.h misc/application.h misc/options.h misc/applicationexecute.h
)
set(CORE_SOURCES
    iannix.cpp iannixapp.cpp iannix_spec.cpp
    misc/help.cpp misc/application.cpp misc/options.cpp misc/applicationexecute.cpp
)

# Messages
set(MESSAGES_HEADERS
    messages/messagemanagerlogmini.h messages/messagemanagerlog.h messages/messagemanager.h messages/message.h messages/messagemanagerloginterface.h
)
set(MESSAGES_SOURCES
    messages/messagemanagerlogmini.cpp messages/messagemanagerlog.cpp messages/messagemanager.cpp messages/message.cpp
)
set(MESSAGES_FORMS
    messages/messagemanagerlogmini.ui messages/messagemanagerlog.ui
)

# Transport
set(TRANSPORT_HEADERS
    transport/transport.h transport/uitimer.h transport/uiabout.h transport/uieditor.h
)
set(TRANSPORT_SOURCES
    transport/transport.cpp transport/uitimer.cpp transport/uiabout.cpp transport/uieditor.cpp
)
set(TRANSPORT_FORMS
    transport/transport.ui transport/uitimer.ui transport/uiabout.ui transport/uieditor.ui
)

# Render
set(RENDER_HEADERS
    render/uirender.h render/uirenderpreview.h
)
set(RENDER_SOURCES
    render/uirender.cpp render/uirenderpreview.cpp
)
set(RENDER_FORMS
    render/uirender.ui
)

# Geometry
set(GEOMETRY_HEADERS
    geometry/nxpoint.h geometry/nxrect.h geometry/nxsize.h geometry/nxline.h geometry/nxpolygon.h geometry/nxeasing.h
    geometry/qmuparser/muParser.h geometry/qmuparser/muParserBase.h geometry/qmuparser/muParserBytecode.h geometry/qmuparser/muParserCallback.h
    geometry/qmuparser/muParserError.h geometry/qmuparser/muParserTokenReader.h geometry/qmuparser/muParserDef.h geometry/qmuparser/muParserFixes.h
    geometry/qmuparser/muParserStack.h geometry/qmuparser/muParserToken.h
)
set(GEOMETRY_SOURCES
    geometry/nxpoint.cpp geometry/nxrect.cpp geometry/nxsize.cpp geometry/nxline.cpp geometry/nxpolygon.cpp geometry/nxeasing.cpp
    geometry/qmuparser/muParser.cpp geometry/qmuparser/muParserBase.cpp geometry/qmuparser/muParserBytecode.cpp geometry/qmuparser/muParserCallback.cpp
    geometry/qmuparser/muParserError.cpp geometry/qmuparser/muParserTokenReader.cpp
)

# Objects
set(OBJECTS_HEADERS
    objects/nxdocument.h objects/nxtrigger.h objects/nxgroup.h objects/nxcurve.h objects/nxcursor.h objects/nxobject.h
)
set(OBJECTS_SOURCES
    objects/nxdocument.cpp objects/nxtrigger.cpp objects/nxgroup.cpp objects/nxcurve.cpp objects/nxcursor.cpp objects/nxobject.cpp
)

# GUI
set(GUI_HEADERS
    gui/uiinspector.h gui/uiview.h gui/uihelp.h gui/uimessagebox.h gui/uisplashscreen.h
    gui/qjsedit/jsedit.h
)
set(GUI_SOURCES
    gui/uiinspector.cpp gui/uiview.cpp gui/uihelp.cpp gui/uimessagebox.cpp gui/uisplashscreen.cpp gui/qjsedit/jsedit.cpp
)
set(GUI_FORMS
    gui/uiinspector.ui gui/uiview.ui gui/uihelp.ui gui/uimessagebox.ui
)

# Items
set(ITEMS_HEADERS
    items/uitreeview.h items/uitreeviewwidget.h items/uitreedelegate.h items/uifileitem.h items/uicoloritem.h items/uipathpointsitem.h items/uitextureitem.h
)
set(ITEMS_SOURCES
    items/uitreeview.cpp items/uitreeviewwidget.cpp items/uitreedelegate.cpp items/uifileitem.cpp items/uicoloritem.cpp items/uipathpointsitem.cpp items/uitextureitem.cpp
)
set(ITEMS_FORMS
    items/uitreeview.ui
)

# Interfaces
set(INTERFACES_HEADERS
    interfaces/extscriptvariableask.h interfaces/extoscpatternask.h interfaces/extoscpatterneditor.h interfaces/interfacesyphon.h
    interfaces/interfacehttp.h interfaces/interfacemidi.h interfaces/interfaceosc.h interfaces/interfaceserial.h interfaces/interfacetcp.h interfaces/interfaceudp.h interfaces/interfacedirect.h
    interfaces/qextserialport/qextserialport.h interfaces/qextserialport/qextserialenumerator.h interfaces/qextserialport/qextserialport_global.h interfaces/qextserialport/qextserialport_p.h interfaces/qextserialport/qextserialenumerator_p.h
    interfaces/qrtmidi/RtMidi.h interfaces/qrtmidi/RtError.h
    interfaces/qwebsockets/websocket.h interfaces/qwebsockets/websocketserver.h interfaces/qwebsockets/websocketprotocol.h interfaces/qwebsockets/handshakerequest.h interfaces/qwebsockets/handshakeresponse.h interfaces/qwebsockets/dataprocessor.h
)
set(INTERFACES_SOURCES
    interfaces/extscriptvariableask.cpp interfaces/extoscpatternask.cpp interfaces/extoscpatterneditor.cpp
    interfaces/interfacehttp.cpp interfaces/interfacemidi.cpp interfaces/interfaceosc.cpp interfaces/interfaceserial.cpp interfaces/interfacetcp.cpp interfaces/interfaceudp.cpp interfaces/interfacedirect.cpp
    interfaces/qextserialport/qextserialport.cpp interfaces/qextserialport/qextserialenumerator.cpp
    interfaces/qrtmidi/RtMidi.cpp
    interfaces/qwebsockets/websocket.cpp interfaces/qwebsockets/websocketserver.cpp interfaces/qwebsockets/websocketprotocol.cpp interfaces/qwebsockets/handshakerequest.cpp interfaces/qwebsockets/handshakeresponse.cpp interfaces/qwebsockets/dataprocessor.cpp
)

if(WIN32)
    list(APPEND INTERFACES_SOURCES 
        interfaces/qextserialport/qextserialport_win.cpp
        interfaces/qextserialport/qextserialenumerator_win.cpp
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND INTERFACES_SOURCES
        interfaces/qextserialport/qextserialport_unix.cpp
        interfaces/qextserialport/qextserialenumerator_linux.cpp
    )
elseif(APPLE)
    list(APPEND INTERFACES_SOURCES
        interfaces/qextserialport/qextserialport_unix.cpp
        interfaces/qextserialport/qextserialenumerator_osx.cpp
    )
else()
    list(APPEND INTERFACES_SOURCES
        interfaces/qextserialport/qextserialport_unix.cpp
        interfaces/qextserialport/qextserialenumerator_unix.cpp
    )
endif()


# AbstractionsGL
set(ABSTRACTIONS_HEADERS
    abstractionsgl.h
)
set(ABSTRACTIONS_SOURCES
    abstractionsgl.cpp
)

# Syphon (macOS only)
if(APPLE)
    list(APPEND SOURCES interfaces/interfacesyphon.mm)
    add_compile_definitions(SYPHON_INSTALLED)
endif()

# Zeroconf / Bonjour (macOS only)
if(APPLE)
    list(APPEND SOURCES
        interfaces/zeroconf/bonjourserviceregister.cpp
        interfaces/zeroconf/bonjourserviceresolver.cpp
        interfaces/zeroconf/bonjourservicebrowser.cpp
    )
    list(APPEND HEADERS
        interfaces/zeroconf/bonjourserviceregister.h
        interfaces/zeroconf/bonjourserviceresolver.h
        interfaces/zeroconf/bonjourservicebrowser.h
    )
endif() 

# Artnet (macOS only)
if(APPLE)
    set(ARTNET_HEADERS
        interfaces/artnet/artnet.h
        interfaces/artnet/misc.h
        interfaces/artnet/packets.h
        interfaces/artnet/private.h
        interfaces/artnet/tod.h
        interfaces/artnet/common.h
    )
    set(ARTNET_SOURCES
        interfaces/artnet/artnet.c
        interfaces/artnet/misc.c
        interfaces/artnet/network.c
        interfaces/artnet/receive.c
        interfaces/artnet/tod.c
        interfaces/artnet/transmit.c
    )
endif() 

# ---- FFmpeg ----


if(USE_FFMPEG)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec libavutil libavformat libswscale
    )

    add_definitions(-DFFMPEG_INSTALLED)

    set(FFMPEG_SOURCES
        gui/qffmpeg/QVideoEncoder.cpp
        gui/gffmpeg/QVideoDecoder.cpp    
    )

    set(FFMPEG_HEADERS
        gui/qffmpeg/QVideoEncoder.h
        gui/qffmpeg/QVideoDecoder.h
    )

    set(FFMPEG_LIBS PkgConfig::FFMPEG)
    
    list(APPEND OPTIONAL_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/qffmpeg
    )

    list(APPEND OPTIONAL_LIBS
        ${FFMPEG_LIBS}
    )

endif()  

# Sources
list(APPEND SOURCES
    ${CORE_SOURCES} 
    ${MESSAGES_SOURCES} 
    ${TRANSPORT_SOURCES} 
    ${RENDER_SOURCES} 
    ${GEOMETRY_SOURCES} 
    ${OBJECTS_SOURCES}
    ${GUI_SOURCES} 
    ${ITEMS_SOURCES} 
    ${INTERFACES_SOURCES} ${INTERFACES_HEADERS}
    ${ARTNET_SOURCES} 
    ${ZEROCONF_SOURCES}
    ${ABSTRACTIONS_SOURCES}
)

# UI files collection (所有需要处理的 .ui)
set(FORMS
    ${MESSAGES_FORMS} ${TRANSPORT_FORMS} ${RENDER_FORMS} ${GUI_FORMS} ${ITEMS_FORMS}
)

# Qt resources
set(RESOURCES
    icons/IanniX.qrc
)

# ---------------------------
# 可执行目标
# ---------------------------
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${FORMS}
    ${RESOURCES}
)

# Include paths 保持原来的路径
target_include_directories(${PROJECT_NAME} PRIVATE
    .
    misc
    messages
    transport
    render
    geometry
    geometry/qmuparser
    objects
    gui
    gui/qjsedit
    items
    interfaces
    interfaces/qextserialport
    interfaces/qrtmidi
    interfaces/qwebsockets
    interfaces/artnet
    interfaces/zeroconf
    ${OPTIONAL_INCLUDES}
)

# ---------------------------
# 链接 Qt（统一使用 QT_LIBS 变量）
# ---------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_LIBS}
    ${OPTIONAL_LIBS}    
)

# ---------------------------
# 平台特定库
# ---------------------------
if(UNIX AND NOT APPLE)
    
    find_package(PkgConfig REQUIRED)
    
    # OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENGL_gl_LIBRARY} GL)
    
    # Alsa
    pkg_check_modules(ALSA REQUIRED alsa)
    target_compile_definitions(${PROJECT_NAME} PRIVATE __LINUX_ALSA__)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ALSA_LIBRARIES} asound)

    # libudev 
    pkg_check_modules(LIBUDEV libudev)
    if(LIBUDEV_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUDEV_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARIES})
        target_compile_options(${PROJECT_NAME} PRIVATE ${LIBUDEV_CFLAGS_OTHERS})
    endif()
elseif(APPLE)
    # macOS 框架
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework CoreMidi"
        "-framework CoreAudio"
        "-framework CoreFoundation"
        "-framework IOKit"
        "-framework Carbon"
        "-framework Cocoa"
        "-framework Syphon"
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE winmm setupapi advapi32 user32 opengl32)
endif()

# ---------------------------
# 安装（模拟 .pro 中的安装路径行为）
# ---------------------------
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# 如果有 .desktop 文件和 icon（在原 .pro 有），按原意安装（可选）
install(FILES iannix.desktop DESTINATION share/applications OPTIONAL)
install(FILES icons/IanniX.png DESTINATION share/pixmaps OPTIONAL)

# ---------------------------
# CMake messages
# ---------------------------
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Configured IanniX (version ${PROJECT_VERSION})")
message(STATUS "  USE_QT_VERSION = ${USE_QT_VERSION}")
message(STATUS "  CMake Generator: ${CMAKE_GENERATOR}")

