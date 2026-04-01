# Builds the lez-explorer-ui library (Qt plugin)
{ pkgs, common, src }:

pkgs.stdenv.mkDerivation {
  pname = "${common.pname}-lib";
  version = common.version;

  inherit src;
  inherit (common) buildInputs cmakeFlags meta;
  nativeBuildInputs = common.nativeBuildInputs;

  # Library (Qt plugin), not an app — no Qt wrapper
  dontWrapQtApps = true;

  configurePhase = ''
    runHook preConfigure
    cmake -S . -B build \
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

    mkdir -p $out/lib
    if [ -f build/modules/lez_explorer_ui.dylib ]; then
      cp build/modules/lez_explorer_ui.dylib $out/lib/
    elif [ -f build/modules/lez_explorer_ui.so ]; then
      cp build/modules/lez_explorer_ui.so $out/lib/
    else
      echo "Error: No library file found in build/modules/"
      ls -la build/modules/ 2>/dev/null || true
      exit 1
    fi

    runHook postInstall
  '';
}
