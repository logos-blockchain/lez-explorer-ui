# Common build configuration shared across all packages
{ pkgs }:

{
  pname = "lez-explorer-ui";
  version = "1.0.0";

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    pkgs.qt6.wrapQtAppsHook
  ];

  buildInputs = [
    pkgs.qt6.qtbase
    pkgs.qt6.qtsvg
    pkgs.qt6.qtwebsockets
  ];

  cmakeFlags = [
    "-GNinja"
  ];

  meta = with pkgs.lib; {
    description = "LEZ Explorer UI - A Qt block explorer for the Logos Execution Zone";
    platforms = platforms.unix;
  };
}
