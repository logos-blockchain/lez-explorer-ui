default: build

# Build the ui_qml module plugin via logos-module-builder (-> result/lib/).
build:
    nix build

# Preview the UI standalone (ui_qml modules run via the builder).
run:
    nix run .

# Drop into the builder dev shell.
develop:
    nix develop

clean:
    rm -rf build result rocksdb-*

# Format the C++ backend.
prettify:
    nix shell nixpkgs#clang-tools -c clang-format -i src/*.cpp src/*.h
