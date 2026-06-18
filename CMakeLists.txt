cmake_minimum_required(VERSION 3.14)
project(LezExplorerUiPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Hand-written Q_OBJECT classes (widgets/pages/services) need MOC;
# explorer_resources.qrc -> compiled icons via AUTORCC.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Logos Module CMake helper. For nix builds this is provided via
# LOGOS_MODULE_BUILDER_ROOT.
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Legacy `type: ui` QWidget plugin (hand-written ExplorerPlugin / IComponent —
# no `interface: universal`, so no codegen). The builder links the logos
# SDK/protocol (logos_api.h / getClient) automatically; we add the Qt widget
# stack the explorer needs on top of the default Core/RemoteObjects.
logos_module(
    NAME lez_explorer_ui
    SOURCES
        src/ExplorerPlugin.cpp
        src/ExplorerPlugin.h
        src/ExplorerWidget.cpp
        src/ExplorerWidget.h
        src/Style.h
        src/services/IndexerService.h
        src/services/LpIndexerService.cpp
        src/services/LpIndexerService.h
        src/services/MockIndexerService.cpp
        src/services/MockIndexerService.h
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
    FIND_PACKAGES
        Qt6Widgets
        Qt6Svg
        Qt6SvgWidgets
        Qt6Network
    LINK_LIBRARIES
        Qt6::Widgets
        Qt6::Svg
        Qt6::SvgWidgets
        Qt6::Network
    INCLUDE_DIRS
        src
        interfaces
)
