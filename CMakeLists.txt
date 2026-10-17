cmake_minimum_required(VERSION 3.16)

# 1. 优先检测系统 VCPKG 并绑定工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(G_Codec VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 消除 Qt Creator 内置 Clang Code Model (clangd) 索引新版 MSVC STL 时的 STL1000 误报
add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)

# 引入自定义 cmake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ==================== 优先寻找 FFMPEG ====================
find_package(FFMPEG REQUIRED)

# ==================== 寻找 QT 库 ====================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    demuxer.h
    demuxer.cpp
    decoder.h
    decoder.cpp
    control_center.h
    control_center.cpp
    transcode_task.h
    task_manager.h
    task_manager.cpp
    save_queue.h
    video_encoder.h
    video_encoder.cpp
    audio_encoder.h
    audio_encoder.cpp
    audio_filter.h
    audio_filter.cpp
    video_filter.h
    video_filter.cpp
    muxer.h
    muxer.cpp
    task_item_widget.h
    task_item_widget.cpp
    task_item_widget.ui
    transcode_dialog.h
    transcode_dialog.cpp
    transcode_dialog.ui
    batch_transcode_dialog.h
    batch_transcode_dialog.cpp
    ffmpeg_global.h
    dark_theme.h
    window_style_helper.h
    test_matrix.h
    resources.qrc
    简要日志
)

if(WIN32)
    list(APPEND PROJECT_SOURCES app.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(G_Codec
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(G_Codec SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(G_Codec
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# ==================== 关联 FFmpeg 头文件目录 ====================
target_include_directories(G_Codec PRIVATE ${FFMPEG_INCLUDE_DIRS})

# ==================== 链接库文件 ====================
target_link_libraries(G_Codec PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    ${FFMPEG_LIBRARIES}
)

if(WIN32)
    target_link_libraries(G_Codec PRIVATE dwmapi)
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.G_Codec)
endif()

set_target_properties(G_Codec PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS G_Codec
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(G_Codec)
endif()

if(MSVC)
    add_compile_options("/utf-8")
endif()
