{
  description = "LEZ Explorer UI - a QML block explorer for the Logos Execution Zone";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    
    lez_indexer_module.url = "git+https://github.com/logos-blockchain/lez-indexer-module?ref=erhant/indexer-fix-bc0.2.3";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
