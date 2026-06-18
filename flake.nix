{
  description = "LEZ Explorer UI - a Qt block explorer for the Logos Execution Zone";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    # The provider it reads from, over the Logos protocol. Input name MUST match
    # the metadata.json "dependencies" entry. For local development against an
    # un-pushed branch, override with a path:
    #   lez_indexer_module.url = "path:../logos-execution-zone-indexer-module";
    # NOTE: git+https (not github:) because the branch name contains a slash —
    # the github: fetcher mis-routes slashed refs through the commits API (404).
    # TODO: repoint to the merge target once this branch lands.
    lez_indexer_module.url = "git+https://github.com/logos-blockchain/logos-execution-zone-indexer-module?ref=erhant/migr-to-logos-module-builder";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
