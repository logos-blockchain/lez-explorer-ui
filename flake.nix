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
    # FIXME: re-pin to main once lez-indexer-module#erhant/0.3.0-release-bumps is merged
    lez_indexer_module.url = "git+https://github.com/logos-blockchain/lez-indexer-module?ref=erhant/0.3.0-release-bumps";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
