cmake_minimum_required(VERSION 3.16)
project(LEZExplorerUIPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg SvgWidgets Network WebSockets)

# Try to find the component-interfaces package first
find_package(component-interfaces QUIET)

# If not found, use the local interfaces folder
if(NOT component-interfaces_FOUND)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
    add_library(component-interfaces INTERFACE)
    target_include_directories(component-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
endif()

# Source files
set(SOURCES
    src/ExplorerPlugin.cpp
    src/ExplorerPlugin.h
    src/ExplorerWidget.cpp
    src/ExplorerWidget.h
    src/Style.h
    src/services/MockIndexerService.cpp
    src/services/MockIndexerService.h
    src/services/RpcIndexerService.cpp
    src/services/RpcIndexerService.h
    src/services/IndexerService.h
    src/models/Block.h
    src/models/Transaction.h
    src/models/Account.h
    src/widgets/SearchBar.cpp
    src/widgets/SearchBar.h
    src/widgets/NavigationBar.cpp
    src/widgets/NavigationBar.h
    src/widgets/ClickableFrame.h
    src/pages/MainPage.cpp
    src/pages/MainPage.h
    src/pages/BlockPage.cpp
    src/pages/BlockPage.h
    src/pages/TransactionPage.cpp
    src/pages/TransactionPage.h
    src/pages/AccountPage.cpp
    src/pages/AccountPage.h
    src/explorer_resources.qrc
)

# Create the plugin library
add_library(lez_explorer_ui SHARED ${SOURCES})

# Set output name without lib prefix
set_target_properties(lez_explorer_ui PROPERTIES
    PREFIX ""
    OUTPUT_NAME "lez_explorer_ui"
)

# Include directories
target_include_directories(lez_explorer_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Link against libraries
target_link_libraries(lez_explorer_ui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Network
    Qt6::WebSockets
    component-interfaces
)

# Set common properties for both platforms
set_target_properties(lez_explorer_ui PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    BUILD_WITH_INSTALL_RPATH TRUE
    SKIP_BUILD_RPATH FALSE
)

if(APPLE)
    set_target_properties(lez_explorer_ui PROPERTIES
        INSTALL_RPATH "@loader_path"
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_NAME_DIR TRUE
    )
    add_custom_command(TARGET lez_explorer_ui POST_BUILD
        COMMAND install_name_tool -id "@rpath/lez_explorer_ui.dylib" $<TARGET_FILE:lez_explorer_ui>
        COMMENT "Updating library paths for macOS"
    )
else()
    set_target_properties(lez_explorer_ui PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE
    )
endif()

install(TARGETS lez_explorer_ui
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/logos/modules
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/metadata.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/lez-explorer-ui
)

message(STATUS "LEZ Explorer UI Plugin configured successfully")
