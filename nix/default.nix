let
  pkgs = import <nixpkgs> { };
in
pkgs.stdenv.mkDerivation {
  pname = "rpcfetch";
  version = "0.0.1";
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
  '';
  makeFlags = with pkgs; [
    "PREFIX=$(out)"
  ];
}
