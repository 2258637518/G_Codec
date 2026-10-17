cmake_minimum_required(VERSION 3.16)

# 1. 优先检测系统 VCPKG 并绑定工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(SlientCodec VERSION 0.1 LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入自定义 cmake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ==================== 【核心改动 1】：优先寻找 FFMPEG（使用全大写，必须在 Qt 之前！） ====================
find_package(FFMPEG REQUIRED)

# ==================== 【核心改动 2】：寻找 QT 库 ====================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(SlientCodec
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}


        demuxer.h demuxer.cpp
        lineitemwidget.h lineitemwidget.cpp
        lineitemwidget.ui
        control_center.h control_center.cpp
        dialog.h dialog.cpp dialog.ui
        decoder.h decoder.cpp
        ffmpeg_global.h
        简要日志
        transcodetask.h
        taskmanager.h taskmanager.cpp
        savequeue.h
        videoencoder.h videoencoder.cpp
        audioencoder.h audioencoder.cpp
        audio_filter.h audio_filter.cpp
        video_filter.h video_filter.cpp
        muxer.h muxer.cpp
    )
else()
    if(ANDROID)
        add_library(SlientCodec SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(SlientCodec
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ==================== 【核心改动 3】：关联 FFmpeg 头文件目录 ====================
target_include_directories(SlientCodec PRIVATE ${FFMPEG_INCLUDE_DIRS})

# ==================== 【核心改动 4】：链接库文件（使用 vcpkg 官方标准的 FFMPEG 变量） ====================
target_link_libraries(SlientCodec PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    ${FFMPEG_LIBRARIES} # vcpkg 会自动将您勾选的 avcodec、avformat、swscale 等库全部打包在这里
)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(SlientCodec PRIVATE Qt${QT_VERSION_MAJOR}::Core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SlientCodec)
endif()
set_target_properties(SlientCodec PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS SlientCodec
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SlientCodec)
endif()

add_compile_options("/utf-8")
