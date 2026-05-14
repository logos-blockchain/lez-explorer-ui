cmake_minimum_required(VERSION 3.16)
project(LEZExplorerUIApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Create the executable
add_executable(lez-explorer-ui-app
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

# Link libraries
target_link_libraries(lez-explorer-ui-app PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# Set RPATH settings for the executable
if(APPLE)
    set_target_properties(lez-explorer-ui-app PROPERTIES
        INSTALL_RPATH "@executable_path/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
elseif(UNIX)
    set_target_properties(lez-explorer-ui-app PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Install rules
install(TARGETS lez-explorer-ui-app
    RUNTIME DESTINATION bin
)
