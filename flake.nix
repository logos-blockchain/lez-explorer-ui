{
  description = "LEZ Explorer UI - a QML block explorer for the Logos Execution Zone";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";

    # NOTE: git+https (not github:) because the branch name contains a slash —
    # the github: fetcher mis-routes slashed refs through the commits API (404).
    # TODO: repoint to the merge target once this branch lands / goes public.
    lez_indexer_module.url = "git+https://github.com/logos-blockchain/lez-indexer-module?ref=erhant/migr-to-logos-module-builder-update-LEZ";
  };

  outputs =
    inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
