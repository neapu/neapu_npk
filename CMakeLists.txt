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
include_directories(${ZLIB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ZLIB_LIBRARIES})