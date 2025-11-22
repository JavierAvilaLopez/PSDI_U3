cmake_minimum_required(VERSION 3.5)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared FileManager library
add_library(FileManager STATIC IMPORTED)
set_target_properties(FileManager PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libFileManager.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/"
)

# Server target
project(server LANGUAGES CXX)
add_executable(server
    server/server.cpp
    server/clientManager.cpp
    server/utils.cpp
)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/server)
target_link_libraries(server pthread FileManager stdc++fs)

# Client target using remote FileManager implementation
project(client LANGUAGES CXX)
add_executable(client
    main_fm.cpp
    client/fileManagerRemoto.cpp
    client/clientManager.cpp
    client/utils.cpp
)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/client)
target_link_libraries(client pthread stdc++fs)

# Local FileManager binary for testing library directly
project(fileManager LANGUAGES CXX)
add_executable(fileManager main_fm.cpp filemanager.h)
target_link_libraries(fileManager pthread FileManager stdc++fs)
