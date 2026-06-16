{
  description = "LEZ Explorer UI - A Qt block explorer for the Logos Execution Zone";

  inputs = {
    # Pinned to the EXACT nixpkgs revision basecamp resolves via logos-nix, so this
    # in-process `type: ui` module links the identical qtbase (6.9.2) and does not
    # load a second QtCore into basecamp's process (which aborts with "Must construct
    # a QApplication before a QWidget"). Matching the version string is not enough —
    # the store path must match, so this must be re-pinned whenever basecamp bumps its
    # logos-nix/nixpkgs lock.
    nixpkgs.url = "github:NixOS/nixpkgs/e9f00bd893984bc8ce46c895c3bf7cac95331127";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      packages = forAllSystems ({ pkgs }:
        let
          common = import ./nix/default.nix { inherit pkgs; };
          src = ./.;

          lib = import ./nix/lib.nix { inherit pkgs common src; };

          app = import ./nix/app.nix {
            inherit pkgs common src;
            lezExplorerUI = lib;
          };
        in
        {
          lez-explorer-ui-lib = lib;
          app = app;
          lib = lib;
          default = lib;
        }
      );

      devShells = forAllSystems ({ pkgs }: {
        default = pkgs.mkShell {
          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
          ];
          buildInputs = [
            pkgs.qt6.qtbase
            pkgs.qt6.qtsvg
            pkgs.qt6.qtwebsockets
          ];

          shellHook = ''
            echo "LEZ Explorer UI development environment"
          '';
        };
      });
    };
}
