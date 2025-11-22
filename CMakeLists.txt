cmake_minimum_required(VERSION 3.10)
project(PSDI_U3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG "Enable debug logging" OFF)
if(DEBUG)
    add_compile_definitions(FILEMGR_DEBUG)
endif()

add_library(FileManager STATIC IMPORTED)
set_target_properties(FileManager PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libFileManager.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/"
)

set(COMMON_SRC rpc_utils.cpp logging.cpp clientManager.cpp)

add_executable(Server server.cpp ${COMMON_SRC})
target_link_libraries(Server pthread FileManager stdc++fs)
target_include_directories(Server PRIVATE ${CMAKE_SOURCE_DIR})

# Client target uses remote FileManager implementation (no libFileManager)
add_executable(Client main_fm.cpp fileManagerRemoto.cpp ${COMMON_SRC})
target_link_libraries(Client pthread stdc++fs)
target_include_directories(Client PRIVATE ${CMAKE_SOURCE_DIR})

add_executable(rpc_tests tests/rpc_tests.cpp ${COMMON_SRC})
target_link_libraries(rpc_tests pthread stdc++fs)
target_include_directories(rpc_tests PRIVATE ${CMAKE_SOURCE_DIR})

enable_testing()
add_test(NAME rpc_roundtrip COMMAND rpc_tests)

