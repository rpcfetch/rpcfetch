let
  pkgs = import <nixpkgs> { };
in
pkgs.stdenv.mkDerivation {
  pname = "rpcfetch";
  version = "0.1.0";
  src = ../.;

  buildInputs = with pkgs; [
    curl
    cjson
    gcc
    pcre2
    unzip
    wget
    xorg.libX11.dev
  ];

  buildPhase = ''
    cp -r ${pkgs.discord-gamesdk}/lib .
    cp -r ${pkgs.discord-gamesdk.dev}/lib/include .
    make offline
  '';

  makeFlags = with pkgs; [
    "PREFIX=$(out)"
  ];
}
