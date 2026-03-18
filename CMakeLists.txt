cmake_minimum_required(VERSION 3.16)
project(QuantumPlayer VERSION 2.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Download miniaudio.h if not present ──────────────────────
set(MINIAUDIO_H "${CMAKE_SOURCE_DIR}/third_party/miniaudio.h")
if(NOT EXISTS "${MINIAUDIO_H}")
    message(STATUS "Downloading miniaudio.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
        "${MINIAUDIO_H}"
        STATUS DL_STATUS SHOW_PROGRESS)
    list(GET DL_STATUS 0 DL_RESULT)
    if(NOT DL_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to download miniaudio.h!\n"
            "Run: download_deps.bat (Windows) or bash download_deps.sh (Linux/macOS)\n"
            "Or manually place miniaudio.h in the third_party/ folder.")
    endif()
    message(STATUS "miniaudio.h downloaded OK")
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets Network)

qt_add_executable(QuantumPlayer
    src/main.cpp
    src/mainwindow.cpp                      src/mainwindow.h

    # Audio engine
    src/player/audioengine.cpp              src/player/audioengine.h
    src/player/miniaudio_impl.cpp
    src/player/replaygain.cpp               src/player/replaygain.h

    # DSP (header only — included via .cpp)

    # UI
    src/ui/visualizerwidget.cpp             src/ui/visualizerwidget.h
    src/ui/equalizerwidget.cpp              src/ui/equalizerwidget.h
    src/ui/playlistwidget.cpp               src/ui/playlistwidget.h
    src/ui/librarywidget.cpp                src/ui/librarywidget.h
    src/ui/nowplayingwidget.cpp             src/ui/nowplayingwidget.h
    src/ui/thememanager.cpp                 src/ui/thememanager.h
    src/ui/levelmeterswidget.cpp            src/ui/levelmeterswidget.h
    src/ui/sleeptimerwidget.cpp             src/ui/sleeptimerwidget.h
    src/ui/clickableslider.h
    src/ui/systemtrayicon.cpp               src/ui/systemtrayicon.h
    src/ui/waveformwidget.cpp               src/ui/waveformwidget.h
    src/ui/lyricswidget.cpp                 src/ui/lyricswidget.h
    src/ui/smartplaylistdialog.cpp          src/ui/smartplaylistdialog.h
    src/ui/tageditordialog.cpp              src/ui/tageditordialog.h
    src/ui/lastfmsettingsdialog.cpp         src/ui/lastfmsettingsdialog.h

    # Network
    src/network/lyricsfetcher.cpp           src/network/lyricsfetcher.h
    src/network/albumartfetcher.cpp         src/network/albumartfetcher.h
    src/network/lastfmscrobbler.cpp         src/network/lastfmscrobbler.h

    # Library
    src/library/track.h
    src/library/album.h
    src/library/artist.h
    src/library/musiclibrary.cpp            src/library/musiclibrary.h
    src/library/folderwatcher.cpp           src/library/folderwatcher.h

    src/resources/resources.qrc
)

target_include_directories(QuantumPlayer PRIVATE src third_party)

target_link_libraries(QuantumPlayer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Platform audio libs for miniaudio
if(WIN32)
    target_link_libraries(QuantumPlayer PRIVATE ole32 user32 advapi32)
elseif(APPLE)
    find_library(COREAUDIO CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX AudioToolbox REQUIRED)
    find_library(COREFOUNDATION CoreFoundation REQUIRED)
    target_link_libraries(QuantumPlayer PRIVATE
        ${COREAUDIO} ${AUDIOTOOLBOX} ${COREFOUNDATION})
else()
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(ALSA QUIET alsa)
        pkg_check_modules(PULSE QUIET libpulse)
    endif()
    if(ALSA_FOUND)
        target_link_libraries(QuantumPlayer PRIVATE ${ALSA_LIBRARIES})
    endif()
    if(PULSE_FOUND)
        target_link_libraries(QuantumPlayer PRIVATE ${PULSE_LIBRARIES})
    endif()
    target_link_libraries(QuantumPlayer PRIVATE pthread dl m)
endif()

set_target_properties(QuantumPlayer PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QuantumPlayer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
