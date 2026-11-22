# VitalRoute - Qt 6 Widgets GUI build.
#
# The Makefile stays the build for the console program and the tests.
# This file builds only the GUI:
#
#   vitalroute_core      static library: C11 engine + C++17 model, modules, app
#   vitalroute_gui_lib   static library: everything in src/gui except main.cpp
#   vitalroute_gui       Qt 6 Widgets front end over HospitalSession
#   vitalroute_gui_check automated checks of the GUI layer (tests/gui/, run by ctest)
#
#   cmake -S . -B build-gui -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-gui
#
# Uses Qt 6 Core, Gui and Widgets only. No network module is linked.
cmake_minimum_required(VERSION 3.16)
project(VitalRoute VERSION 3.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------- core --
# Same sources the Makefile compiles, minus the console's main.cpp.
file(GLOB VR_ENGINE_SOURCES CONFIGURE_DEPENDS src/engine/*.c)
file(GLOB VR_CXX_SOURCES CONFIGURE_DEPENDS
     src/model/*.cpp src/modules/*.cpp src/app/*.cpp)

add_library(vitalroute_core STATIC ${VR_ENGINE_SOURCES} ${VR_CXX_SOURCES})
target_include_directories(vitalroute_core PUBLIC
    src/engine src/model src/modules src/app)
if(NOT WIN32)
    find_library(VR_MATH_LIBRARY m)
    if(VR_MATH_LIBRARY)
        target_link_libraries(vitalroute_core PUBLIC ${VR_MATH_LIBRARY})
    endif()
endif()

# ----------------------------------------------------------------- GUI --
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Every GUI source except main.cpp goes into a library, so the app and the
# automated check program (tests/gui_check.cpp) run exactly the same code.
file(GLOB VR_GUI_SOURCES CONFIGURE_DEPENDS src/gui/*.cpp src/gui/*.hpp)
list(FILTER VR_GUI_SOURCES EXCLUDE REGEX "/src/gui/main\\.cpp$")
file(GLOB VR_GUI_RESOURCES CONFIGURE_DEPENDS src/gui/*.qrc)

add_library(vitalroute_gui_lib STATIC ${VR_GUI_SOURCES})
target_include_directories(vitalroute_gui_lib PUBLIC src/gui)
target_link_libraries(vitalroute_gui_lib PUBLIC vitalroute_core Qt6::Core Qt6::Gui Qt6::Widgets)
target_compile_definitions(vitalroute_gui_lib PUBLIC
    QT_NO_KEYWORDS                 # signals/slots spelled Q_SIGNALS / Q_SLOTS
    QT_NO_CAST_FROM_ASCII          # every std::string <-> QString conversion is explicit
    VR_SOURCE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Resources (the stylesheet) are compiled into each executable directly.
add_executable(vitalroute_gui src/gui/main.cpp ${VR_GUI_RESOURCES})
target_link_libraries(vitalroute_gui PRIVATE vitalroute_gui_lib)
# No console window behind the GUI on Windows.
set_target_properties(vitalroute_gui PROPERTIES WIN32_EXECUTABLE ON)

# ------------------------------------------------------- GUI checks --
# Automated checks of the GUI layer (a console program, not part of the app):
# drives the real windows on temporary copies of data/. See tests/gui/.
#
#   ctest --test-dir build-gui --output-on-failure
#   build-gui/vitalroute_gui_check [--shots DIR] [--only GROUP,...]
file(GLOB VR_GUI_CHECK_SOURCES CONFIGURE_DEPENDS tests/gui/*.cpp tests/gui/*.hpp)
add_executable(vitalroute_gui_check ${VR_GUI_CHECK_SOURCES} ${VR_GUI_RESOURCES})
target_include_directories(vitalroute_gui_check PRIVATE tests/gui)
target_link_libraries(vitalroute_gui_check PRIVATE vitalroute_gui_lib)

enable_testing()
add_test(NAME gui_check COMMAND vitalroute_gui_check)
# Off screen: no windows appear while ctest runs.
set_tests_properties(gui_check PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 300)
