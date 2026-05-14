{
  description = "LEZ Explorer UI - A Qt block explorer for the Logos Execution Zone";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
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
          ];

          shellHook = ''
            echo "LEZ Explorer UI development environment"
          '';
        };
      });
    };
}
