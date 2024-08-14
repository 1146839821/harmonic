#!/usr/bin/env sh

rm -rf build
meson setup --cross-file mingw_cross.txt $* build
