#!/bin/bash

# Setup script for Discord Game SDK libs and headers

if ! command -v wget &> /dev/null; then
    echo "wget is required to download Discord Game SDK"
    exit
fi

if ! command -v unzip &> /dev/null; then
    echo "unzip is required to unzip the downloaded file"
    exit
fi

rm -rf tmp
rm -rf lib
rm -f include/discord_game_sdk.h
mkdir tmp
mkdir lib

wget "https://dl-game-sdk.discordapp.net/3.2.1/discord_game_sdk.zip" -O tmp/discord_game_sdk.zip
unzip tmp/discord_game_sdk.zip -d tmp/discord_game_sdk

cp tmp/discord_game_sdk/lib/x86_64/*.so lib/
cp tmp/discord_game_sdk/c/discord_game_sdk.h include/

echo "Successfully set up Discord Game SDK"
