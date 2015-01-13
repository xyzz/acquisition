#!/bin/bash

rm -rf deploy
mkdir deploy
cd deploy
windeployqt.exe ../release/acquisition.exe --release --no-compiler-runtime --dir .
cp ../release/acquisition.exe .
cp ../redist/* .
rm -rf bearer
echo "[Paths]\nLibraries=./plugins" > qt.conf
cd ..
