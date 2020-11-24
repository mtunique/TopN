cmake_minimum_required(VERSION 3.0.0)
project(pocotest VERSION 0.1.0)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

include_directories(src)
file(GLOB Sources "*.cpp")

message(src ${Sources})

add_executable(main ${Sources})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
