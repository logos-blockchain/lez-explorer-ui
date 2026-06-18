{
  description = "LEZ Explorer UI - a Qt block explorer for the Logos Execution Zone";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    # The provider it reads from, over the Logos protocol. Input name MUST match
    # the metadata.json "dependencies" entry. For local development against an
    # un-pushed branch, override with a path:
    #   lez_indexer_module.url = "path:../logos-execution-zone-indexer-module";
    # TODO: use the correct branch & repo name afterwards
    lez_indexer_module.url = "github:logos-blockchain/logos-execution-zone-indexer-module/erhant/migr-to-logos-module-builder";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
