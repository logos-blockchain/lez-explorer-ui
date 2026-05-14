# LEZ Explorer UI

A Qt6/C++ block explorer for the **Logos Execution Zone** (LEZ). Builds as both a shared library (Qt plugin) and a standalone desktop application.

## Features

- Browse recent blocks with live streaming of new blocks
- View block details: hash, previous hash, timestamp, status, signature, transactions
- Navigate to previous blocks via clickable hash links
- View transaction details (Public, Privacy-Preserving, Program Deployment)
- View account details with paginated transaction history
- Full-text search by block ID, block hash, transaction hash, or account ID
- Browser-style back/forward navigation
- Pagination with "Load more" for older blocks
- Dark theme (Catppuccin Mocha)

## Building with Nix

### Standalone application

```sh
nix build '.#app'
nix run '.#app'
```

### Shared library (Qt plugin)

```sh
nix build '.#lib'
```

### Development shell

```sh
nix develop
```

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE-v2](LICENSE-APACHE-v2))
- MIT License ([LICENSE-MIT](LICENSE-MIT))

at your option.
