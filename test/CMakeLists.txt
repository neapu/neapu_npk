CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(npk_test)
add_executable(${PROJECT_NAME} test.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(${PROJECT_NAME} npk)