# Usage:
# cmake -P GenerateMappings.cmake <path/to/mappings.h.in> <path/to/mappings.h>

set(source_url "https://gist.githubusercontent.com/afxgroup/28fd4bf01c95512a4da21bc5de0ba0b5/raw/b499e822e28ca8bb3d9ff52fb3a0c6402e9a1901/gamecontrollerdb.txt")
set(source_path "${CMAKE_CURRENT_BINARY_DIR}/gamecontrollerdb.txt")
set(template_path "${CMAKE_ARGV3}")
set(target_path "${CMAKE_ARGV4}")

if (NOT EXISTS "${template_path}")
    message(FATAL_ERROR "Failed to find template file ${template_path}")
endif()

file(DOWNLOAD "${source_url}" "${source_path}"
     STATUS download_status
     TLS_VERIFY on)

list(GET download_status 0 status_code)
list(GET download_status 1 status_message)

if (status_code)
    message(FATAL_ERROR "Failed to download ${source_url}: ${status_message}")
endif()

file(STRINGS "${source_path}" lines)
foreach(line ${lines})
    if (line MATCHES "^[0-9a-fA-F]")
        if (line MATCHES "platform:Windows")
            if (GLFW_WIN32_MAPPINGS)
                string(APPEND GLFW_WIN32_MAPPINGS "\n")
            endif()
            string(APPEND GLFW_WIN32_MAPPINGS "\"${line}\",")
        elseif (line MATCHES "platform:Mac OS X")
            if (GLFW_COCOA_MAPPINGS)
                string(APPEND GLFW_COCOA_MAPPINGS "\n")
            endif()
            string(APPEND GLFW_COCOA_MAPPINGS "\"${line}\",")
        elseif (line MATCHES "platform:Linux")
            if (GLFW_LINUX_MAPPINGS)
                string(APPEND GLFW_LINUX_MAPPINGS "\n")
            endif()
            string(APPEND GLFW_LINUX_MAPPINGS "\"${line}\",")
        elseif (line MATCHES "platform:AmigaOS4")
            if (GLFW_OS4_MAPPINGS)
                string(APPEND GLFW_OS4_MAPPINGS "\n")
            endif()
            string(APPEND GLFW_OS4_MAPPINGS "\"${line}\",")
        endif()
    endif()
endforeach()

configure_file("${template_path}" "${target_path}" @ONLY NEWLINE_STYLE UNIX)
file(REMOVE "${source_path}")

