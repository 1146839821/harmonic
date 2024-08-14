#!/usr/bin/env bash
set -e

strip="x86_64-w64-mingw32-strip"

rm -f harmonic.zip
rm -rf out

sh setup.sh --buildtype=release
meson compile -C build

mkdir out

cp ./build/injector/harmonic_dumper.exe ./out
cp ./build/injector/harmonic_loader.exe ./out
cp ./build/injector/launcher_payload.dll ./out
cp ./build/game_payload/game_payload.dll ./out
cp ./LICENSE.txt ./out

$strip ./out/*.{exe,dll}

if [ "x$1" = "xrelease" ]; then
    cd out
    zip ../harmonic.zip *
fi
