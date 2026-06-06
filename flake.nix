{
  description = "Reproducible Nix build for pokeemerald-expansion-shared-power";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          commonNativeBuildInputs = [
            pkgs.bash
            pkgs.gcc
            pkgs.gcc-arm-embedded
            pkgs.git
            pkgs.gnumake
            pkgs.perl
            pkgs.pkg-config
            pkgs.python3
          ];
          commonBuildInputs = [
            pkgs.libpng
            pkgs.zlib
          ];
        in
        rec {
          mgba-rom-test = pkgs.stdenv.mkDerivation {
            pname = "mgba-rom-test";
            version = pkgs.mgba.version;

            src = pkgs.mgba.src;

            strictDeps = true;

            nativeBuildInputs = [
              pkgs.cmake
              pkgs.pkg-config
            ];

            buildInputs = [
              pkgs.elfutils
              pkgs.libedit
              pkgs.libpng
              pkgs.libzip
              pkgs.lua5_2
              pkgs.minizip
              pkgs.sqlite
              pkgs.zlib
            ];

            cmakeFlags = [
              "-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.10"
              "-DBUILD_ROM_TEST=ON"
              "-DBUILD_QT=OFF"
              "-DBUILD_SDL=OFF"
              "-DBUILD_LIBRETRO=OFF"
              "-DBUILD_TEST=OFF"
              "-DBUILD_SUITE=OFF"
              "-DBUILD_CINEMA=OFF"
              "-DBUILD_PERF=OFF"
              "-DBUILD_EXAMPLE=OFF"
              "-DBUILD_PYTHON=OFF"
              "-DUSE_DISCORD_RPC=OFF"
              "-DUSE_FFMPEG=OFF"
            ];

            postInstall = ''
              test -x "$out/bin/mgba-rom-test"
            '';

            meta = {
              description = "mGBA ROM test runner";
              platforms = systems;
            };
          };

          mgba-rom-test-hydra = pkgs.stdenv.mkDerivation {
            pname = "mgba-rom-test-hydra";
            version = "0-unstable";

            src = ./tools/mgba-rom-test-hydra;

            strictDeps = true;

            nativeBuildInputs = [
              pkgs.gcc
              pkgs.gnumake
            ];

            buildPhase = ''
              runHook preBuild

              make clean
              make

              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall

              install -Dm755 mgba-rom-test-hydra "$out/bin/mgba-rom-test-hydra"

              runHook postInstall
            '';

            meta = {
              description = "Hydra wrapper for mGBA ROM test output";
              platforms = systems;
            };
          };

          pokeemerald = pkgs.stdenv.mkDerivation {
            pname = "pokeemerald-expansion-shared-power";
            version = "0-unstable";

            src = ./.;

            strictDeps = true;

            nativeBuildInputs = commonNativeBuildInputs;

            buildInputs = commonBuildInputs;

            DEVKITARM = pkgs.gcc-arm-embedded;

            dontConfigure = true;
            enableParallelBuilding = true;

            buildPhase = ''
              runHook preBuild

              touch .histignore
              make tidymodern clean-tools clean-check-tools
              make -j"$NIX_BUILD_CORES" NO_MULTIBOOT=1

              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall

              install -Dm444 pokeemerald.gba "$out/pokeemerald.gba"
              install -Dm444 pokeemerald.elf "$out/pokeemerald.elf"
              install -Dm444 pokeemerald.map "$out/pokeemerald.map"

              runHook postInstall
            '';

            meta = {
              description = "Shared Power pokeemerald-expansion ROM build";
              platforms = systems;
            };
          };

          pokeemerald-test-smoke = pkgs.stdenv.mkDerivation {
            pname = "pokeemerald-expansion-shared-power-test-smoke";
            version = "0-unstable";

            src = ./.;

            strictDeps = true;

            nativeBuildInputs = commonNativeBuildInputs ++ [
              mgba-rom-test
              mgba-rom-test-hydra
            ];

            buildInputs = commonBuildInputs;

            DEVKITARM = pkgs.gcc-arm-embedded;

            dontConfigure = true;
            enableParallelBuilding = true;

            buildPhase = ''
              runHook preBuild

              touch .histignore
              make tidycheck clean-tools clean-check-tools
              make -j"$NIX_BUILD_CORES" NO_MULTIBOOT=1 TESTS="uq4_12" check

              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall

              mkdir -p "$out"
              printf 'make check TESTS="uq4_12" passed under Nix\n' > "$out/check-passed.txt"

              runHook postInstall
            '';

            meta = {
              description = "Shared Power pokeemerald-expansion Nix test-runner smoke check";
              platforms = systems;
            };
          };

          default = pokeemerald;
        }
      );

      checks = forAllSystems (system: {
        rom-build = self.packages.${system}.pokeemerald;
        test = self.packages.${system}.pokeemerald-test-smoke;
      });

      devShells = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          packages = self.packages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.gcc
              pkgs.gcc-arm-embedded
              pkgs.git
              pkgs.gnumake
              pkgs.perl
              pkgs.pkg-config
              pkgs.python3
              pkgs.libpng
              pkgs.zlib
              packages.mgba-rom-test
              packages.mgba-rom-test-hydra
            ];

            DEVKITARM = pkgs.gcc-arm-embedded;
            ROMTEST = "${packages.mgba-rom-test}/bin/mgba-rom-test";
          };
        }
      );
    };
}
