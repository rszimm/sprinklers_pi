cmake_minimum_required(VERSION 3.9)
project(sprinklers_pi)

set(CMAKE_CXX_STANDARD 11)

file (STRINGS "version.txt" BUILD_VERSION)

add_definitions(-DLOGGING)
add_definitions(-DVERSION=\"${BUILD_VERSION}\")
add_definitions(-DRELPATH)

add_executable(sprinklers_pi
        core.cpp
        core.h
        Event.cpp
        Event.h
        Logging.cpp
        Logging.h
        port.cpp
        port.h
        settings.cpp
        settings.h
        sprinklers_pi.cpp
        sysreset.cpp
        sysreset.h
        Weather.cpp
        Weather.h
        Wunderground.cpp
        Wunderground.h
        web.cpp
        web.h)

TARGET_LINK_LIBRARIES(sprinklers_pi
        sqlite3
        wiringPi
        crypt
        rt)

set (source "${CMAKE_SOURCE_DIR}/scripts")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/scripts")
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
)
set (source "${CMAKE_SOURCE_DIR}/web")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/web")
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
)