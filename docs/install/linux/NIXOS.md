# NixOS instructions

This repository includes a Nix flake for reproducible ROM builds.

Build the ROM from the repository root:

```bash
nix build
```

The build output is written to `result/` and includes `pokeemerald.gba`, `pokeemerald.elf`, and `pokeemerald.map`.

For an interactive development shell with the ARM toolchain and native helper-tool dependencies:

```bash
nix develop
```

Inside the shell, use the normal Makefile commands. If the working tree already contains helper binaries built outside Nix, clean and rebuild them first:

```bash
make clean-tools clean-check-tools
make -j"$(nproc)" NO_MULTIBOOT=1
```

Run a focused repository test smoke through the Nix-provided mGBA ROM test runner:

```bash
nix develop -c make NO_MULTIBOOT=1 TESTS="uq4_12" check
```

Run the full repository test suite through the same Nix runner:

```bash
nix develop -c make NO_MULTIBOOT=1 check
```

Run all flake checks, including the ROM build and full Nix test suite:

```bash
nix flake check
```
