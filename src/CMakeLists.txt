cmake_minimum_required(VERSION 3.20)
project(npk)
add_library(${PROJECT_NAME} STATIC
        NPKHandler.cpp
        NPKHandler.h
        NPKImageHandler.cpp
        NPKImageHandler.h
        NPKFrameHandler.cpp
        NPKFrameHandler.h
        NPKMatrix.cpp
        NPKMatrix.h
        NPKPaletteManager.cpp
        NPKPaletteManager.h
        NPKPublic.h
        NPKDDSHandler.cpp
        NPKDDSHandler.h
        NPKPublic.cpp
)
find_package(ZLIB REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ZLIB_LIBRARIES})
message("LOGGER_INCLUDE_DIR " ${LOGGER_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${LOGGER_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC logger)
if(NOT DISABLE_OPENSSL)
    add_compile_definitions(USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
    message("OPENSSL_INCLUDE_DIR " ${OPENSSL_INCLUDE_DIR} " OPENSSL_LIBRARIES " ${OPENSSL_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})
    if (MSVC)
        target_link_libraries(${PROJECT_NAME} PUBLIC crypt32 ws2_32)
    endif ()
endif()
if (NOT DISABLE_PNG)
    add_compile_definitions(USE_PNG)
    find_package(PNG REQUIRED)
    message("PNG_INCLUDE_DIRS " ${PNG_INCLUDE_DIRS} " PNG_LIBRARIES " ${PNG_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${PNG_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${PNG_LIBRARIES})
endif ()