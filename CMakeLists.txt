cmake_minimum_required(VERSION 3.5.0)
project(Trading-System VERSION 0.1.0 LANGUAGES C CXX)

add_executable(Trading-System main.cpp)

set_property(TARGET Trading-System PROPERTY CXX_STANDARD 23)
set_property(TARGET Trading-System PROPERTY CXX_STANDARD_REQUIRED ON)


include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
