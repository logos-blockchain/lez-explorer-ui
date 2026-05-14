# Builds the lez-explorer-ui-app standalone application
{ pkgs, common, src, lezExplorerUI }:

pkgs.stdenv.mkDerivation rec {
  pname = "lez-explorer-ui-app";
  version = common.version;

  inherit src;
  inherit (common) buildInputs meta;

  nativeBuildInputs = common.nativeBuildInputs ++ pkgs.lib.optionals pkgs.stdenv.isLinux [ pkgs.patchelf ];

  # This is a GUI application, enable Qt wrapping
  dontWrapQtApps = false;

  qtLibPath = pkgs.lib.makeLibraryPath ([
    pkgs.qt6.qtbase
  ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
    pkgs.libglvnd
  ]);

  qtPluginPath = "${pkgs.qt6.qtbase}/lib/qt-6/plugins:${pkgs.qt6.qtsvg}/lib/qt-6/plugins";

  qtWrapperArgs = [
    "--prefix" "LD_LIBRARY_PATH" ":" qtLibPath
    "--prefix" "QT_PLUGIN_PATH" ":" qtPluginPath
  ];

  configurePhase = ''
    runHook preConfigure
    cmake -S app -B build \
      -GNinja \
      -DCMAKE_BUILD_TYPE=Release
    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild
    cmake --build build
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p $out/bin $out/lib

    # Install the app binary
    cp build/bin/lez-explorer-ui-app $out/bin/

    # Determine platform-specific plugin extension
    OS_EXT="so"
    case "$(uname -s)" in
      Darwin) OS_EXT="dylib";;
    esac

    # Copy the explorer UI plugin to root directory (loaded relative to binary)
    if [ -f "${lezExplorerUI}/lib/lez_explorer_ui.$OS_EXT" ]; then
      cp -L "${lezExplorerUI}/lib/lez_explorer_ui.$OS_EXT" "$out/"
    fi

    runHook postInstall
  '';
}
