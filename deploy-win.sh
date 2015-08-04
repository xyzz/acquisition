#!/bin/bash

rm -rf deploy
mkdir deploy
cd deploy
windeployqt.exe ../release/acquisitionplus.exe --release --no-compiler-runtime --dir .
cp ../release/acquisitionplus.exe .
cp ../redist/* .
rm -rf bearer
echo "[Paths]\nLibraries=./plugins" > qt.conf
cd ..
