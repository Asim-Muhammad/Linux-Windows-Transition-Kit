cmake_minimum_required(VERSION 3.24)
project(fedora_win_explorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # Enable LTO (-flto)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(win10_thispc SHARED win10_thispc_helper.cpp)
set_target_properties(win10_thispc PROPERTIES PREFIX "lib" OUTPUT_NAME "win10_thispc")
target_compile_options(win10_thispc PRIVATE -O2 -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(win10_thispc PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets ${CMAKE_DL_LIBS})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dump_qrc.cpp")
    add_executable(dump_qrc dump_qrc.cpp)
    target_compile_options(dump_qrc PRIVATE -O2 -Wall -Wextra)
    target_link_libraries(dump_qrc PRIVATE Qt6::Core ${CMAKE_DL_LIBS})
endif()
