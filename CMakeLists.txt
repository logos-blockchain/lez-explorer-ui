cmake_minimum_required(VERSION 3.14)
project(LezExplorerUiPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Logos Module CMake helper. For nix builds this is provided via
# LOGOS_MODULE_BUILDER_ROOT.
if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found. Set LOGOS_MODULE_BUILDER_ROOT.")
endif()

# Universal ui_qml module: we author only the .rep view contract + the *Backend
# class. REP_FILE runs repc; the generated *Plugin / *Interface glue in
# generated_code/ (Q_PLUGIN_METADATA, initLogos wiring, QtRO registration) is
# compiled automatically. The QML view (src/qml/) is delivered by the host
# alongside the plugin — no Qt Widgets, no .qrc. The logos SDK/protocol (incl.
# the typed lez_indexer_module wrappers) is linked by the builder.
logos_module(
    NAME lez_explorer_ui
    REP_FILE src/lez_explorer_ui.rep
    SOURCES
        src/lez_explorer_ui_backend.h
        src/lez_explorer_ui_backend.cpp
    INCLUDE_DIRS
        src
)
