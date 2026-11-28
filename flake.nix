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
    # logos-execution-zone-module#83 (fees, receipts, async start, richer getStatus); back to main once it merges.
    lez_core.url = "git+https://github.com/logos-blockchain/logos-execution-zone-module?rev=7bf906459f53f0ed324a7a0ee57159552f43b0a3";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
