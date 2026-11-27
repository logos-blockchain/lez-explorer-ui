{
  description = "LEZ Explorer UI - a QML block explorer for the Logos Execution Zone";

  # Pull pre-built artifacts from the self-hosted Logos Attic cache(Nix binary cache).
  nixConfig = {
    extra-substituters = [ "https://cache.nix.logos.co/public" ];
    extra-trusted-public-keys = [ "public:l4HrXgL4nw246+LBh2SOJyhz64BoGegOYLheT/iIAPU=" ];
  };

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    # logos-execution-zone-module#82 (lez_core 0.7.0: setup/start from a home); back to main once it merges.
    lez_core.url = "git+https://github.com/logos-blockchain/logos-execution-zone-module?rev=0d458e31923c0bcf7f306e145cb641bc3d6c395b";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
