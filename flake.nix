{
  description = "neofetch in discord";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    (flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
      in rec
      {
        packages.rpcfetch = pkgs.stdenv.mkDerivation {
          pname = "rpcfetch";
          version = "0.1.0";
          src = ./.;

          buildInputs = with pkgs; [
            curl
            gcc
            pcre2
            xorg.libX11.dev
          ];

          buildPhase = ''
            cp -r ${pkgs.discord-gamesdk}/lib .
            cp -r ${pkgs.discord-gamesdk.dev}/lib/include .
          '';

          makeFlags = with pkgs; [
            "PREFIX=$(out)"
            "CC=gcc"
          ];
        };
        defaultPackage = packages.rpcfetch;

        devShells.default = pkgs.mkShell {
          name = "rpcfetch development shell";
          buildInputs = with pkgs; [
            curl
            gcc
            gnumake
            pcre2
            unzip
            wget
            xorg.libX11.dev
          ];
        };
      }
    ));
}
