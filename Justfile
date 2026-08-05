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

# Unit-test the status parsing (Qt6Core only, no module build needed).
test-backend:
    mkdir -p build
    nix shell nixpkgs#qt6.qtbase nixpkgs#pkg-config --command bash -c \
        'clang++ -std=c++17 -Isrc tests/sync_status_test.cpp $(pkg-config --cflags --libs Qt6Core) -o build/sync_status_test && ./build/sync_status_test'

# Format the C++ backend.
prettify:
    nix shell nixpkgs#clang-tools -c clang-format -i src/*.cpp src/*.h
