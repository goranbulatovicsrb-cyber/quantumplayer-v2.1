# Downloads miniaudio.h if not present
set(MINIAUDIO_H "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio.h")
if(NOT EXISTS "${MINIAUDIO_H}")
    message(STATUS "Downloading miniaudio.h ...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
        "${MINIAUDIO_H}"
        STATUS DOWNLOAD_STATUS
        SHOW_PROGRESS
    )
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
    if(NOT DOWNLOAD_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download miniaudio.h. "
            "Please manually download from "
            "https://github.com/mackron/miniaudio/blob/master/miniaudio.h "
            "and place it in the 'third_party/' folder.")
    endif()
    message(STATUS "miniaudio.h downloaded successfully.")
endif()

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
